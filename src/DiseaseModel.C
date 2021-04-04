/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "DiseaseModel.h"
#include "Event.h"
#include "Defs.h"

#include "disease_model/disease.pb.h"
#include "disease_model/distribution.pb.h"
#include "readers/DataReader.h"

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
const double TRANSMISSIBILITY = .00002 / DAY_LENGTH;

/**
 * Constructor which loads in disease file from text proto file.
 * On failure, aborts the entire simulation.
 */
DiseaseModel::DiseaseModel(std::string pathToModel) {
  // Load in text proto definition.
  // TODO(iancostello): Load directly without string.
  model = new loimos::proto::DiseaseModel();
  std::ifstream diseaseModelStream(pathToModel);
  if (!diseaseModelStream) {
    CkAbort("Could not read disease model at %s.", pathToModel.c_str());
  }
  std::string str((std::istreambuf_iterator<char>(diseaseModelStream)),
                  std::istreambuf_iterator<char>());
  if (!google::protobuf::TextFormat::ParseFromString(str, model)) {
    CkAbort("Could not parse protobuf!");
  }
  diseaseModelStream.close();

  // Setup other shared PE objects.
  personDef = new loimos::proto::CSVDefinition();
  std::ifstream personInputStream(scenarioPath + "people.textproto");
  std::string strPerson((std::istreambuf_iterator<char>(personInputStream)),
                  std::istreambuf_iterator<char>());
  if (!google::protobuf::TextFormat::ParseFromString(strPerson, personDef)) {
    CkAbort("Could not parse protobuf!");
  }
  personInputStream.close();
  locationDef = new loimos::proto::CSVDefinition();
  std::ifstream locationInputStream(scenarioPath + "locations.textproto");
  std::string strLocation((std::istreambuf_iterator<char>(locationInputStream)),
                  std::istreambuf_iterator<char>());
  if (!google::protobuf::TextFormat::ParseFromString(strLocation, locationDef)) {
    CkAbort("Could not parse protobuf!");
  }
  locationInputStream.close();
  activityDef = new loimos::proto::CSVDefinition();
  std::ifstream activityInputStream(scenarioPath + "visits.textproto");
  std::string strActivity((std::istreambuf_iterator<char>(activityInputStream)),
                  std::istreambuf_iterator<char>());
  if (!google::protobuf::TextFormat::ParseFromString(strActivity, activityDef)) {
    CkAbort("Could not parse protobuf!");
  }
  activityInputStream.close();
}

/**
 * Returns the name of the state at a given index
 */
std::string DiseaseModel::lookupStateName(int state) const {
  return model->disease_state(state).state_label();
}

/**
 * Handles a disease state transition given a set of edges to use. This function
 * should be called when the user needs to make a state transition.
 *
 * When people enter a new state, they need to determine both the next state
 * and the time until they will transition to that new state.
 * 
 * Args:
 *  fromState: The state the person is currently in.
 *  nextState: The state that the person is transitioning into. 
 *  
 * Returns:
 *  What state they will transition out of fromState into and how much time
 * they will spend there. 
 */
std::tuple<int, int> DiseaseModel::transitionFromState(
  int fromState,
  std::default_random_engine *generator
) const {
  // Get current state and next transition set to use.
  const loimos::proto::DiseaseModel_DiseaseState *currState =
      &model->disease_state(fromState);

  // Two cases
  if (currState->has_timed_transition()) {
    // printf("Timed trans from %d\n", fromState);
    // Get the next transition set to use.
    // Currently for timed transitions we only support one set edge.
    const loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet
        *transition_set = &(currState->timed_transition());
    // Check if any transitions to be made.
    int transitionSetSize = transition_set->transition_size();
    if (transitionSetSize == 0) {
      return std::make_tuple(fromState, std::numeric_limits<Time>::max());
    }

    // Randomly choose a state transition from the set and return the next state.
    float cdfSoFar = 0;
    std::uniform_real_distribution<float> uniform_dist(0, 1);
    float randomCutoff = uniform_dist(*generator);
    for (int i = 0; i < transitionSetSize; i++) {
      const loimos::proto::
          DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
              *transition = &transition_set->transition(i);
      // TODO: Create a CDF vector in initialization.
      cdfSoFar += transition->with_prob();
      if (randomCutoff <= cdfSoFar) {
        int nextState = transition->next_state();
        int timeInNextState = getTimeInNextState(transition, generator);
        return std::make_tuple(nextState, timeInNextState);
      }
    }
    // A state transition should be made.
    CkAbort("No state transition made! From state %d.", fromState);
  } else if (currState->has_exposure_transition()) {
    return std::make_tuple(
      currState->exposure_transition().transition(0).next_state(), 0);
  } else {
    return std::make_tuple(fromState, std::numeric_limits<Time>::max());
  }
}

/**
 * Calculates the time to spend in the next state.
 */
Time DiseaseModel::getTimeInNextState(
  const loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition *transitionSet,
  std::default_random_engine *generator
) const {
  if (transitionSet->has_fixed()) {
    return timeDefToSeconds(transitionSet->fixed().time_in_state());

  } else if (transitionSet->has_forever()) {
    return std::numeric_limits<Time>::max();

  } else if (transitionSet->has_uniform()) {
    // TODO(iancostello): Avoid reinitializing the distribution each time.
    std::uniform_real_distribution<double> uniform_dist(
      timeDefToSeconds(transitionSet->uniform().tmin()),
      timeDefToSeconds(transitionSet->uniform().tmax())
    );
    return uniform_dist(*generator);
  } else if (transitionSet->has_normal()) {
    std::normal_distribution<double> normal_dist(
      timeDefToSeconds(transitionSet->normal().tmean()),
      timeDefToSeconds(transitionSet->normal().tvariance())
    );
    return normal_dist(*generator);
  } else if (transitionSet->has_discrete()) {
    std::uniform_real_distribution<float> uniform_dist(0, 1);
    float randomCutoff = uniform_dist(*generator);
    float cdfSoFar = 0;
    for (int i = 0; i < transitionSet->discrete().bin_size(); i++) {
      cdfSoFar += transitionSet->discrete().bin(i).with_prob();
      if (randomCutoff < cdfSoFar) {
        return timeDefToSeconds(transitionSet->discrete().bin(i).tval());
      }
    }
  }
  return 0;
}

/** Converts a protobuf time definition into a seconds as an integer */
Time DiseaseModel::timeDefToSeconds(Time_Def time) const {
  return static_cast<Time>(time.days() * DAY_LENGTH +
         time.hours() * HOUR_LENGTH + 
         time.minutes() * MINUTE_LENGTH);
}

/** Returns the total number of disease states */
int DiseaseModel::getNumberOfStates() const {
  return model->disease_state_size();
}

/** Returns the initial starting healthy and exposed state */
int DiseaseModel::getHealthyState(std::vector<Data> dataField) const { 
  // Age based transition.
  int personAge = dataField[AGE_CSV_INDEX].int_b10;
  int numStartingStates = model->starting_states_size();
  for (int stateNum = 0; stateNum < numStartingStates; stateNum++) {
    const loimos::proto::DiseaseModel_StartingCondition state = 
      model->starting_states(stateNum); 

    if (state.lower() <= personAge &&
        personAge <= state.upper()) {
      return state.starting_state();
    }
  }
  CkAbort("No starting state for person of age %d. Read %d states total.", personAge, numStartingStates);  
}

/** Returns if someone is infectious */
bool DiseaseModel::isInfectious(int personState) const { 
  return model->disease_state(personState).infectivity() != 0.0;
}

/** Returns if someone is susceptible */
bool DiseaseModel::isSusceptible(int personState) const {
  return model->disease_state(personState).susceptibility() != 0.0;
}

/** Returns the name of the person's state, as a C-style string */
const char * DiseaseModel::getStateLabel(int personState) const {
  return model->disease_state(personState).state_label().c_str();
}

/** 
 * Returns the natural log of the probability of a suspectible person not being
 * infected by an infectious person after a period of time
 */
double DiseaseModel::getLogProbNotInfected(
  Event susceptibleEvent,
  Event infectiousEvent
) const {
  
  // The chance of being infected in a unit of time depends on...
  double baseProb = 1.0 -
    // ...a scaling factor (normalizes based on the unit of time)...
    TRANSMISSIBILITY
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

/**
 * Returns the propesity of a person in susceptibleState becoming infected 
 * after exposure to a person in infectiousState for the period from startTime
 * to endTime
 */
double DiseaseModel::getPropensity(
  int susceptibleState,
  int infectiousState,
  int startTime,
  int endTime
) const {
  int dt = endTime - startTime;

  // EpiHiper had a number of weights/scaling constants that we may add in 
  // later, but for now we ommit most of them (which is equivalent to setting
  // them all to one)
  return TRANSMISSIBILITY
    * dt
    * model->disease_state(susceptibleState).susceptibility()
    * model->disease_state(infectiousState).infectivity();
}
