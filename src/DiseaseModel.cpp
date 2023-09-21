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
#include "Person.h"
#include "Location.h"
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

  if (0 == CkMyNode()) {
    for (int i = 0; i < model->disease_states_size(); ++i) {
      CkPrintf("  Disease state %d: sus=%f, inf=%f\n",
        i, model->disease_states(i).susceptibility(),
        model->disease_states(i).infectivity());
    }
  }

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
    ageIdx = DataReader<>::getAttributeIndex(personDef, "age");
#if ENABLE_DEBUG >= DEBUG_VERBOSE
    if (0 == CkMyNode()) {
    CkPrintf("  Age to be stored at index %d\n",
        ageIdx);
    }
#endif

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

    if (static_cast<int>(ContactModelType::min_max_alpha) == contactModelType) {
      maxSimVisitsIdx = DataReader<>::getAttributeIndex(locationDef,
          "max_simultaneous_visits");
      if (-1 == maxSimVisitsIdx) {
        CkAbort("Error: required attribute \"max_simultaneous_visits\" not present\n");
      } else {
#if ENABLE_DEBUG >= DEBUG_VERBOSE
        if (0 == CkMyNode()) {
          CkPrintf("  Max sim visit count to be stored at index %d\n",
              maxSimVisitsIdx);
        }
#endif
      }
    }

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

    personAttributes.readAttributes(personDef->fields());
    locationAttributes.readAttributes(locationDef->fields());
  }

  if (interventionStategy) {
    interventionDef = new loimos::proto::InterventionModel();
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

    triggerFlags.resize(interventionDef->triggers_size(), false);

    personAttributes.readAttributes(interventionDef->person_attributes());
    locationAttributes.readAttributes(interventionDef->location_attributes());

    intitialisePersonInterventions(interventionDef->person_interventions(),
        personAttributes);
    intitialiseLocationInterventions(
        interventionDef->location_interventions(),
        locationAttributes);
  }

  susceptibilityIndex = personAttributes.getAttributeIndex("susceptibility");
  infectivityIndex = personAttributes.getAttributeIndex("infectivity");
}

void DiseaseModel::intitialisePersonInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes) {
  for (uint i = 0; i < interventionSpecs.size(); ++i) {
    const loimos::proto::InterventionModel::Intervention &spec =
      interventionSpecs[i];

    if (spec.has_self_isolation()) {
      personInterventions.emplace_back(new SelfIsolationIntervention(
        spec, *model, attributes));

    } else if (spec.has_vaccination()) {
      personInterventions.emplace_back(new VaccinationIntervention(
        spec, *model, attributes));
    }
  }
}

void DiseaseModel::intitialiseLocationInterventions(
    const InterventionList &interventionSpecs,
    const AttributeTable &attributes) {
  for (uint i = 0; i < interventionSpecs.size(); ++i) {
    const loimos::proto::InterventionModel::Intervention &spec =
      interventionSpecs[i];

    if (spec.has_school_closures()) {
      locationInterventions.emplace_back(new SchoolClosureIntervention(
        spec, *model, attributes));
    }
  }
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

  } else if (-1 == ageIdx) {
    CkAbort("No age data (needed for determinign healthy disease state)\n");
  }

  // Age based transition.
  int personAge = dataField[ageIdx].int32_val;
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
  double result = model->transmissibility() * dt * susceptibility * infectivity
    * model->disease_states(susceptibleState).susceptibility()
    * model->disease_states(infectiousState).infectivity() / DAY_LENGTH;

  // CkPrintf("  Propensity %f: transmissibility=%f, dt=%d, sus=%d(%f,%f), inf=%d(%f,%f)\n",
  //     result, model->transmissibility(), dt, susceptibleState, susceptibility,
  //     model->disease_states(susceptibleState).susceptibility(),
  //     infectiousState, infectivity,
  //     model->disease_states(infectiousState).infectivity());
  return result;
}

/**
 * Intervention Methods
 * TODO: Move to own chare as these become more complex.
 */


const Intervention<Person> & DiseaseModel::getPersonIntervention(int index)
  const {
  return *personInterventions[index];
}

const Intervention<Location> & DiseaseModel::getLocationIntervention(int index)
  const {
  return *locationInterventions[index];
}

int DiseaseModel::getNumPersonInterventions() const {
  return static_cast<int>(personInterventions.size());
}

int DiseaseModel::getNumLocationInterventions() const {
  return static_cast<int>(locationInterventions.size());
}

void DiseaseModel::applyInterventions(int day, Id newDailyInfections) {
  toggleInterventions(day, newDailyInfections);

  for (uint i = 0; i < personInterventions.size(); ++i) {
    if (triggerFlags[personInterventions[i]->getTriggerIndex()]) {
      peopleArray.ReceiveIntervention(i);
    }
  }
  for (uint i = 0; i < locationInterventions.size(); ++i) {
    if (triggerFlags[locationInterventions[i]->getTriggerIndex()]) {
      locationsArray.ReceiveIntervention(i);
    }
  }
}

void DiseaseModel::toggleInterventions(int day, Id newDailyInfections) {
  for (uint i = 0; i < interventionDef->triggers_size(); ++i) {
    const loimos::proto::InterventionModel::Trigger &trigger =
      interventionDef->triggers(i);

    // Trigger uses simulation day
    if (trigger.has_day()) {
      const auto &tmp = trigger.day();
      triggerFlags[i] = (!triggerFlags[i] && tmp.trigger_on() <= day)
        || (triggerFlags[i] && tmp.trigger_off() <= day);

    // Trigger uses number of new cases
    } else if (trigger.has_new_daily_cases()) {
      double infectionRate = static_cast<double>(newDailyInfections)
        / numPeople;
      const auto &tmp = trigger.new_daily_cases();
      triggerFlags[i] =
        (!triggerFlags[i] && tmp.trigger_on() <= infectionRate)
        || (triggerFlags[i] && tmp.trigger_off() <= infectionRate);
    }
  }
}
