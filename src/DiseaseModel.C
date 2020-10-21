/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "disease.pb.h"
#include "distribution.pb.h"

#include "loimos.decl.h"
#include "DiseaseModel.h"
#include "Event.h"

#include <cstdio>
#include <cmath>
#include <fstream>
#include <google/protobuf/text_format.h>
#include <random>
#include <limits>

using NameIndexLookupType = std::unordered_map<std::string, int>;

// This is currently used to adjust the infection probability so that
// not everyone gets infected immediately given the small time units
// (i.e. it normalizes for the size of the smallest time increment used
// in the discrete event simulation and disease model)
const double INFECTION_PROBABILITY = 1.0 / DAY_LENGTH;

/**
 * Constructor which loads in disease file from text proto file.
 * On failure, aborts the entire simulation.
 */
DiseaseModel::DiseaseModel(std::string pathToModel) {
  // Load in text proto definition.
  // TODO(iancostello): Load directly without string.
  model = new loimos::proto::DiseaseModel();
  std::ifstream t(pathToModel);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  if (!google::protobuf::TextFormat::ParseFromString(str, model)) {
    CkAbort("Could not parse protobuf!");
  }

  // Create an intervention strategy mapping for fast lookup.
  // TODO(iancostello): Remove manual allocation in the future.
  // TODO(iancostello): ADD deconstructor.
  strategy_lookup = new std::vector<NameIndexLookupType *>;
  state_lookup = new NameIndexLookupType;
  for (int stateIndex = 0; stateIndex < model->disease_state_size();
       stateIndex++) {
    // Get next state.
    const loimos::proto::DiseaseModel_DiseaseState *curr_state =
        &model->disease_state(stateIndex);

    // Add to lookup map.
    NameIndexLookupType *treatment_map = new NameIndexLookupType();
    state_lookup->insert({curr_state->state_label(), stateIndex});
    for (int transitionIndex = 0;
         transitionIndex < curr_state->transition_set_size();
         transitionIndex++) {
      treatment_map->insert(
          {curr_state->transition_set(transitionIndex).transition_label(),
           transitionIndex});
    }
    strategy_lookup->push_back(treatment_map);
  }

  // Init commonly used states.
  healthyState = getIndexOfState("uninfected");
}

/**
 * Returns the initial uninfected state, as an int.
 * Args:
 *      stateLabel: Name of a disease state (e.g. uninfected)
 * Returns:
 *      Index of that state in the loaded disease model.
 */
int DiseaseModel::getIndexOfState(std::string stateLabel) {
  return state_lookup->at(stateLabel);
}

/**
 * Returns the name of the state at a given index
 */
std::string DiseaseModel::lookupStateName(int state) {
  return model->disease_state(state).state_label();
}

/**
 * Handles a disease state transition given a set of edges to use. This function
 * should be called when the user needs to make a state transition.
 *
 * Args:
 *  fromState: The current state of a person in the disease.
 * Returns:
 *  The next state to transition to as an int.
 */
std::tuple<int, int> DiseaseModel::transitionFromState(int fromState,
                                  std::string interventionStategy,
                                  std::default_random_engine *generator) {
  // Get current state and next transition set to use.
  const loimos::proto::DiseaseModel_DiseaseState *curr_state =
      &model->disease_state(fromState);

  // Get the next transition set to use.
  const loimos::proto::DiseaseModel_DiseaseState_StateTransitionSet
      *transition_set = &(curr_state->transition_set(
          strategy_lookup->at(fromState)->at(interventionStategy)));

  // Randomly choose a state transition from the set and return the next state.
  float cdfSoFar = 0;
  std::uniform_real_distribution<float> uniform_dist(0, 1);
  float randomCutoff = uniform_dist(*generator);

  for (int i = 0; i < transition_set->transition_size(); i++) {
    const loimos::proto::
        DiseaseModel_DiseaseState_StateTransitionSet_StateTransition
            *transition = &transition_set->transition(i);
    // TODO: Create a CDF vector in initialization.
    cdfSoFar += transition->with_prob();
    if (randomCutoff <= cdfSoFar) {
      int nextState = state_lookup->at(transition->next_state());
      int timeInNextState = getTimeInNextState(nextState, generator);
      return std::make_tuple(nextState, timeInNextState);
    }
  }

  // A state transition should be made.
  CkAbort("No state transition made!");
}

/**
 * Calculates the time to spend in the next state.
 */
Time DiseaseModel::getTimeInNextState(int nextState,
                                      std::default_random_engine *generator) {
  const loimos::proto::DiseaseModel_DiseaseState *diseaseState =
      &model->disease_state(nextState);
  if (diseaseState->has_fixed()) {
    return timeDefToSeconds(diseaseState->fixed().time_in_state());

  } else if (diseaseState->has_forever()) {
    return std::numeric_limits<Time>::max();

  } else if (diseaseState->has_uniform()) {
    // TODO(iancostello): Avoid reinitializing the distribution each time.
    double lower = timeDefToSeconds(diseaseState->uniform().tmin());
    double upper = timeDefToSeconds(diseaseState->uniform().tmax());
    std::uniform_real_distribution<double> uniform_dist(lower, upper);
    return uniform_dist(*generator);
  }
  // TODO(iancostello): Implement other strategies
  return 0;
}

/** Converts a protobuf time definition into a seconds as an integer */
Time DiseaseModel::timeDefToSeconds(Time_Def time) {
  return (time.days() * 24 + time.hours()) * 3600 + time.minutes() * 60;
}

/** Returns the total number of disease states */
int DiseaseModel::getNumberOfStates() { return model->disease_state_size(); }

/** Returns the initial starting state */
int DiseaseModel::getHealthyState() { return healthyState; }

/** Returns if someone is infectious */
bool DiseaseModel::isInfectious(int personState) { 
  return model->disease_state(personState).infectivity() != 0.0;
}

/** Returns if someone is susceptible */
bool DiseaseModel::isSusceptible(int personState) { 
  return model->disease_state(personState).susceptibility() != 0.0;
}

/** Returns the name of the person's state, as a C-style string */
const char * DiseaseModel::getStateLabel(int personState) {
  return model->disease_state(personState).state_label().c_str();
}

/** 
 * Returns the natural log of the probability of a suspectible person not being
 * infected by an infectious person after a period of time
 */
double DiseaseModel::getLogProbNotInfected(Event susceptibleEvent, Event infectiousEvent) {
  
  // The chance of being infected in a unit of time depends on...
  double baseProb = 1.0 -
    // ...a scaling factor (normalizes based on the unit of time)...
    INFECTION_PROBABILITY
    // ...the susceptibility of the susceptible person...
    * model->disease_state(susceptibleEvent.personState).susceptibility()
    // ...and the infectivity of the infectious person
    * model->disease_state(infectiousEvent.personState).infectivity();
  
  // The probability of not being infected in a period of time is decided based
  // on a geometric probability distribution, with the lenght of time the two
  // people are in the same location serving as the number of trials
  int dt = abs(susceptibleEvent.scheduledTime - infectiousEvent.scheduledTime);
  
  return log(baseProb) * dt;
}
