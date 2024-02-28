/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * The disease model class
 *
 * The disease model class provides the a timed finite state automata
 *
 * This class provides an abstraction over the core proto definition.
 * .transition
 */

#include "DiseaseModel.h"
#include "Defs.h"
#include "Event.h"
#include "Person.h"
#include "Location.h"
#include "readers/Preprocess.h"
#include "readers/DataReader.h"
#include "readers/AttributeTable.h"
#include "contact_model/ContactModel.h"
#include "intervention_model/Intervention.h"
#include "intervention_model/VaccinationIntervention.h"
#include "intervention_model/SelfIsolationIntervention.h"
#include "intervention_model/SchoolClosureIntervention.h"
#include "protobuf/interventions.pb.h"
#include "protobuf/disease.pb.h"
#include "protobuf/distribution.pb.h"
#include "protobuf/data.pb.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <google/protobuf/text_format.h>
#include <limits>
#include <random>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <unordered_map>

// This is currently used to adjust the infection probability so that
// not everyone gets infected immediately given the small time units
// (i.e. it normalizes for the size of the smallest time increment used
// in the discrete event simulation and disease model)

/**
 * Constructor which loads in disease file from text proto file.
 * On failure, aborts the entire simulation.
 */
DiseaseModel::DiseaseModel(std::string diseasePath, const AttributeTable &attrs) {
  model = new loimos::proto::DiseaseModel();
  readProtobuf(diseasePath, model);
  assert(model->disease_states_size() != 0);

  ageIndex = attrs.getAttributeIndex("age");
  susceptibilityIndex = attrs.getAttributeIndex("susceptibility");
  infectivityIndex = attrs.getAttributeIndex("infectivity");

  // if (0 == CkMyNode()) {
  //   for (int i = 0; i < model->disease_states_size(); ++i) {
  //     CkPrintf("  Disease state %d: sus=%f, inf=%f\n",
  //       i, model->disease_states(i).susceptibility(),
  //       model->disease_states(i).infectivity());
  //   }
  // }
}

/**
 * Returns the name of the state at a given index
 */
std::string DiseaseModel::lookupStateName(DiseaseState state) const {
  return model->disease_states(state).state_label();
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
std::tuple<DiseaseState, Time>
DiseaseModel::transitionFromState(DiseaseState fromState,
    std::default_random_engine *generator) const {
  // Get current state and next transition set to use.
  const loimos::proto::DiseaseModel_DiseaseState *currState =
    &model->disease_states(fromState);

  // Two cases
  if (currState->has_timed_transition()) {
    // Get the next transition set to use.
    // Currently for timed transitions we only support one set edge.
    const loimos::proto::DiseaseModel_DiseaseState_TimedTransitionSet
      *transition_set = &(currState->timed_transition());

    // Check if any transitions to be made.
    uint transitionSetSize = transition_set->transitions_size();
    if (transitionSetSize == 0) {
      return std::make_tuple(fromState, std::numeric_limits<Time>::max());
    }

    // Randomly choose a state transition from the set and return the next
    // state.
    float cdfSoFar = 0;
    std::uniform_real_distribution<float> uniform_dist(0, 1);
    float randomCutoff = uniform_dist(*generator);
    for (uint i = 0; i < transitionSetSize; i++) {
      const loimos::proto::
        DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
        *transition = &transition_set->transitions(i);

      // TODO(IanCostello): Create a CDF vector in initialization.
      cdfSoFar += transition->with_prob();
      if (randomCutoff <= cdfSoFar) {
        DiseaseState nextState = static_cast<DiseaseState>(transition->next_state());
        Time timeInNextState = getTimeInNextState(transition, generator);
        // CkPrintf("  Transitioning from %d to %d\n", fromState, nextState);
        return std::make_tuple(nextState, timeInNextState);
      }
    }
    // A state transition should be made.
    CkAbort("No state transition made! From state %d.", fromState);

  } else if (currState->has_exposure_transition()) {
    DiseaseState nextState = static_cast<DiseaseState>(
      currState->exposure_transition()
        .transitions(0)
        .next_state());
    // CkPrintf("  Transitioning from %d to %d\n", fromState, nextState);
    return std::make_tuple(nextState, 0);

    /*
    // If already infected then they are settling in this state so no transition.
    if (alreadyInfected) {
      return std::make_tuple(fromState, std::numeric_limits<Time>::max());
    } else {
      // Otherwise they are transitioning out of this state.
      return std::make_tuple(
        currState->exposure_transition().transition(0).next_state(), 0);
    }
    */
  } else {
    // CkPrintf("  State %s is forever\n", currState->state_label().c_str());
    return std::make_tuple(fromState, std::numeric_limits<Time>::max());
  }
}

/**
 * Calculates the time to spend in the next state.
 */
Time DiseaseModel::getTimeInNextState(
    const loimos::proto::
    DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
    *transitionSet,
    std::default_random_engine *generator) const {

  if (transitionSet->has_fixed()) {
    return timeDefToSeconds(transitionSet->fixed().time_in_state());

  } else if (transitionSet->has_forever()) {
    return std::numeric_limits<Time>::max();

  } else if (transitionSet->has_uniform()) {
    // TODO(iancostello): Avoid reinitializing the distribution each time.
    std::uniform_real_distribution<double> uniform_dist(
        timeDefToSeconds(transitionSet->uniform().tmin()),
        timeDefToSeconds(transitionSet->uniform().tmax()));
    return uniform_dist(*generator);

  } else if (transitionSet->has_normal()) {
    std::normal_distribution<double> normal_dist(
        timeDefToSeconds(transitionSet->normal().tmean()),
        timeDefToSeconds(transitionSet->normal().tvariance()));
    return normal_dist(*generator);

  } else if (transitionSet->has_discrete()) {
    std::uniform_real_distribution<float> uniform_dist(0, 1);
    float randomCutoff = uniform_dist(*generator);
    float cdfSoFar = 0;

    for (uint i = 0; i < transitionSet->discrete().bins_size(); i++) {
      cdfSoFar += transitionSet->discrete().bins(i).with_prob();

      if (randomCutoff < cdfSoFar) {
        return timeDefToSeconds(transitionSet->discrete().bins(i).tval());
      }
    }
  }
  return 0;
}

/** Converts a protobuf time definition into a seconds as an integer */
Time DiseaseModel::timeDefToSeconds(TimeDef time) const {
  return static_cast<Time>(time.days() * DAY_LENGTH
      + time.hours() * HOUR_LENGTH
      + time.minutes() * MINUTE_LENGTH);
}

/** Returns the total number of disease states */
int DiseaseModel::getNumberOfStates() const {
  return model->disease_states_size();
}

/** Returns the initial starting healthy and exposed state */
DiseaseState DiseaseModel::getHealthyState(const std::vector<Data> &dataField) const {
  DiseaseState numStartingStates = model->starting_states_size();

  // Shouldn't need to check age if there's only one starting state
  if (1 == numStartingStates) {
    const loimos::proto::DiseaseModel_StartingCondition state =
      model->starting_states(0);
    return static_cast<DiseaseState>(state.starting_state());

  } else if (-1 == ageIndex) {
    CkAbort("No age data (needed for determinign healthy disease state)\n");
  }

  // Age based transition.
  int personAge = dataField[ageIndex].int32_val;
  for (uint stateNum = 0; stateNum < numStartingStates; stateNum++) {
    const loimos::proto::DiseaseModel_StartingCondition state =
      model->starting_states(stateNum);

    if (state.lower() <= personAge && personAge <= state.upper()) {
      return static_cast<DiseaseState>(state.starting_state());
    }
  }
  CkAbort("No starting state for person of age %d. Read %d states total.",
      personAge, numStartingStates);
}

/** Returns if someone is infectious */
bool DiseaseModel::isInfectious(DiseaseState personState) const {
  return model->disease_states(personState).infectivity() != 0.0;
}

/** Returns if someone is susceptible */
bool DiseaseModel::isSusceptible(DiseaseState personState) const {
  return model->disease_states(personState).susceptibility() != 0.0;
}

/** Returns the name of the person's state, as a C-style string */
const char *DiseaseModel::getStateLabel(DiseaseState personState) const {
  return model->disease_states(personState).state_label().c_str();
}

/**
 * Returns the natural log of the probability of a suspectible person not being
 * infected by an infectious person after a period of time
 */
double DiseaseModel::getLogProbNotInfected(Event susceptibleEvent,
    Event infectiousEvent) const {

  // The chance of being infected in a unit of time depends on...
  double baseProb =
    1.0 -
    // ...a scaling factor (normalizes based on the unit of time)...
    model->transmissibility()
    // ...the susceptibility of the susceptible person...
    * model->disease_states(susceptibleEvent.personState).susceptibility()
    // ...and the infectivity of the infectious person
    * model->disease_states(infectiousEvent.personState).infectivity();

  // The probability of not being infected in a period of time is decided based
  // on a geometric probability distribution, with the lenght of time the two
  // people are in the same location serving as the number of trials
  Time dt = abs(susceptibleEvent.scheduledTime - infectiousEvent.scheduledTime);
  return log(baseProb) * dt;
}

/**
 * Returns the propensity of a person in susceptibleState becoming infected
 * after exposure to a person in infectiousState for the period from startTime
 * to endTime
 */
double DiseaseModel::getPropensity(DiseaseState susceptibleState,
    DiseaseState infectiousState, Time startTime, Time endTime,
    double susceptibility, double infectivity)
    const {
  Time dt = endTime - startTime;

  // EpiHiper had a number of weights/scaling constants that we may add in
  // later, but for now we omit most of them (which is equivalent to setting
  // them all to one)
  return model->transmissibility() * dt * susceptibility * infectivity
    * model->disease_states(susceptibleState).susceptibility()
    * model->disease_states(infectiousState).infectivity() / DAY_LENGTH;
}
