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

#include "loimos.decl.h"
#include "DiseaseModel.h"
#include "Defs.h"
#include "Extern.h"
#include "Event.h"
#include "readers/DataReader.h"
#include "disease_model/disease.pb.h"
#include "disease_model/distribution.pb.h"
#include "readers/interventions.pb.h"
#include "AttributeTable.h"
#include "Person.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <google/protobuf/text_format.h>
#include <limits>
#include <random>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

using NameIndexLookupType = std::unordered_map<std::string, int>;

// This is currently used to adjust the infection probability so that
// not everyone gets infected immediately given the small time units
// (i.e. it normalizes for the size of the smallest time increment used
// in the discrete event simulation and disease model)

/**
 * Constructor which loads in disease file from text proto file.
 * On failure, aborts the entire simulation.
 */
DiseaseModel::DiseaseModel(std::string pathToModel, std::string scenarioPath,
    std::string pathToIntervention) {
  // Load in text proto definition.
  // TODO(iancostello): Load directly without string.

  // Load Disease model
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
  assert(model->disease_states_size() != 0);



  int personTableSize = 0;
  int locationTableSize = 0;

  // Setup other shared PE objects.
  if (!syntheticRun) {
    // Handle people...
    personDef = new loimos::proto::CSVDefinition();
    std::ifstream personInputStream(scenarioPath + "people.textproto");
    if (!personInputStream)
      CkAbort("Could not open people textproto!");
    std::string strPerson((std::istreambuf_iterator<char>(personInputStream)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(strPerson, personDef)) {
      CkAbort("Could not parse person protobuf!");
    }
    personInputStream.close();

    // ...locations...
    locationDef = new loimos::proto::CSVDefinition();
    std::ifstream locationInputStream(scenarioPath + "locations.textproto");
    if (!locationInputStream)
      CkAbort("Could not open location textproto!");
    std::string strLocation((std::istreambuf_iterator<char>(
            locationInputStream)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(strLocation,
          locationDef)) {
      CkAbort("Could not parse location protobuf!");
    }
    locationInputStream.close();

    // ...and visits
    activityDef = new loimos::proto::CSVDefinition();
    std::ifstream activityInputStream(scenarioPath + "visits.textproto");
    if (!activityInputStream)
      CkAbort("Could not open activity textproto!");
    std::string strActivity((std::istreambuf_iterator<char>(
            activityInputStream)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(strActivity,
          activityDef)) {
      CkAbort("Could not parse activity protobuf!");
    }
    activityInputStream.close();

    personTable.readData(personDef);
    locationTable.readData(locationDef);
  }

  //Read in other info besides size -- data type and dummy default value

  personTable.populateTable("att");
  locationTable.populateTable("att");

  if (interventionStategy) {
    interventionDef = new loimos::proto::Intervention();
    std::ifstream interventionActivityStream(pathToIntervention);
    if (!interventionActivityStream)
      CkAbort("Could not open intervention textproto!");
    std::string interventionString((std::istreambuf_iterator<char>(
            interventionActivityStream)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(interventionString,
          interventionDef)) {
      CkAbort("Could not parse protobuf!");
    }
    interventionActivityStream.close();
  }

  // Always toggle intervention off to start.
  interventionToggled = false;
}

/**
 * Returns the name of the state at a given index
 */
std::string DiseaseModel::lookupStateName(int state) const {
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
std::tuple<int, int>
DiseaseModel::transitionFromState(int fromState,
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
    int transitionSetSize = transition_set->transitions_size();
    if (transitionSetSize == 0) {
      return std::make_tuple(fromState, std::numeric_limits<Time>::max());
    }

    // Randomly choose a state transition from the set and return the next
    // state.
    float cdfSoFar = 0;
    std::uniform_real_distribution<float> uniform_dist(0, 1);
    float randomCutoff = uniform_dist(*generator);
    for (int i = 0; i < transitionSetSize; i++) {
      const loimos::proto::
        DiseaseModel_DiseaseState_TimedTransitionSet_StateTransition
        *transition = &transition_set->transitions(i);

      // TODO(IanCostello): Create a CDF vector in initialization.
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
        currState->exposure_transition().transitions(0).next_state(), 0);

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

    for (int i = 0; i < transitionSet->discrete().bins_size(); i++) {
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
int DiseaseModel::getHealthyState(const std::vector<Data> &dataField) const {
  int numStartingStates = model->starting_states_size();

  // Shouldn't need to check age if there's only one starting state
  if (1 == numStartingStates) {
    const loimos::proto::DiseaseModel_StartingCondition state =
      model->starting_states(0);
    return state.starting_state();

  } else if (AGE_CSV_INDEX >= dataField.size()) {
    CkAbort("No age data (needed for determining healthy disease state\n");
  }

  // Age based transition.
  int personAge = dataField[AGE_CSV_INDEX].int_b10;
  for (int stateNum = 0; stateNum < numStartingStates; stateNum++) {
    const loimos::proto::DiseaseModel_StartingCondition state =
      model->starting_states(stateNum);

    if (state.lower() <= personAge && personAge <= state.upper()) {
      return state.starting_state();
    }
  }
  CkAbort("No starting state for person of age %d. Read %d states total.",
      personAge, numStartingStates);
}

/** Returns if someone is infectious */
bool DiseaseModel::isInfectious(int personState) const {
  return model->disease_states(personState).infectivity() != 0.0;
}

/** Returns if someone is susceptible */
bool DiseaseModel::isSusceptible(int personState) const {
  return model->disease_states(personState).susceptibility() != 0.0;
}

/** Returns the name of the person's state, as a C-style string */
const char *DiseaseModel::getStateLabel(int personState) const {
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
  int dt = abs(susceptibleEvent.scheduledTime - infectiousEvent.scheduledTime);
  return log(baseProb) * dt;
}

/**
 * Returns the propesity of a person in susceptibleState becoming infected
 * after exposure to a person in infectiousState for the period from startTime
 * to endTime
 */
double DiseaseModel::getPropensity(int susceptibleState, int infectiousState,
    int startTime, int endTime) const {
  int dt = endTime - startTime;

  // EpiHiper had a number of weights/scaling constants that we may add in
  // later, but for now we ommit most of them (which is equivalent to setting
  // them all to one)
  return model->transmissibility() * dt
    * model->disease_states(susceptibleState).susceptibility()
    * model->disease_states(infectiousState).infectivity();
}

/**
 * Intervention Methods
 * TODO: Move to own chare as these become more complex.
 */
void DiseaseModel::toggleIntervention(int newDailyInfections) {
  if (!interventionToggled) {
    if (static_cast<double>(newDailyInfections) / numPeople >=
          interventionDef->new_daily_cases_trigger_on()) {
      interventionToggled = true;
      printf("Intervention toggled!\n");
    }
  } else {
    if (static_cast<double>(newDailyInfections) / numPeople <=
          interventionDef->new_daily_cases_trigger_off()) {
      interventionToggled = false;
    }
  }
}

/**
 * For now only the self-isolation intervention has a compilance value
 */
double DiseaseModel::getCompilance() const {
  if (interventionStategy && interventionDef->stay_at_home()) {
    return interventionDef->isolation_compliance();
  } else {
    return 0;
  }
}

/**
 * Only thing that causes person to self-isolate is if interventions are
 * triggered, that intervention imposes stay at home, and person is symptomatic.
 */
bool DiseaseModel::shouldPersonIsolate(int healthState) {
  return interventionToggled
    && interventionDef->stay_at_home()
    && model->disease_states(healthState).symptomatic();
}

/**
 * Location closed if it is a school and intervention is triggered.
 */
bool DiseaseModel::isLocationOpen(std::vector<Data> *locAttr) const {
  return !(interventionToggled && interventionDef->school_closures() &&
    locAttr->at(interventionDef->csv_location_of_school()).int_b10 > 0);
}

bool DiseaseModel::complyingWithLockdown(std::default_random_engine *generator) const {
  std::uniform_real_distribution<double> uniform_dist(0, 1);
  return uniform_dist(*generator) < interventionDef->school_closure_compliance();
}
