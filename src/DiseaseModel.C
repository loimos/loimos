/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "DiseaseModel.h"
#include "disease.pb.h"
#include "distribution.pb.h"

#include <stdio.h>
#include <fstream>
#include <google/protobuf/text_format.h>

/**
 * Constructor which loads in disease file from text proto.
 */
DiseaseModel::DiseaseModel(std::string pathToModel) {
    // Load in text proto definition.
    // TODO(iancostello): Load directly without string.
    // CkPrintf("Initializing");
    model = new loimos::proto::DiseaseModel();
    std::ifstream t(pathToModel);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    assert(google::protobuf::TextFormat::ParseFromString(str, model));

    // Create an intervention strategy mapping for fast lookup.
    stategy_lookup = new std::vector<std::map<std::string, int> *>();
    state_lookup = new std::map<std::string, int>();
    for (int stateIndex = 0; stateIndex < model->disease_state_size();
        stateIndex++) {
        // Get next state.
        loimos::proto::DiseaseModel_DiseaseState curr_state =
            model->disease_state(stateIndex);

        // Add to lookup map.
        std::map<std::string, int> *intervention_map = new std::map<std::string, int>();
        state_lookup->insert({curr_state.state_label(), stateIndex});
        for (int transitionIndex = 0;
                transitionIndex < curr_state.transition_set_size();
                transitionIndex++) {
            intervention_map->insert(
                {curr_state.transition_set(transitionIndex).transition_label(), transitionIndex});
        }
        stategy_lookup->push_back(intervention_map);
    }
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
std::tuple<int, int> DiseaseModel::transitionFromState(int fromState, std::string interventionStategy, std::default_random_engine generator) {
  // Get current state and next transition set to use.
  loimos::proto::DiseaseModel_DiseaseState curr_state =
      model->disease_state(fromState);

  // Get the next transition set to use.
  loimos::proto::DiseaseModel_DiseaseState_StateTransitionSet transition_set =
      (curr_state.transition_set(
          stategy_lookup->at(fromState)->at(interventionStategy)));

  // Randomly choose a state transition from the set and return the next state.
  float cdfSoFar = 0;
  std::uniform_real_distribution<float> uniform_dist(0, 1);
  float randomCutoff = uniform_dist(generator); 

  for (int i = 0; i < transition_set.transition_size(); i++) {
    loimos::proto::DiseaseModel_DiseaseState_StateTransitionSet_StateTransition
        transition = transition_set.transition(i);
    cdfSoFar += transition.with_prob();
    if (randomCutoff <= cdfSoFar) {
      int nextState = state_lookup->at(transition.next_state());
      int timeInNextState = getTimeInNextState(nextState, generator);
      return std::make_tuple(nextState, timeInNextState);
     }
  }

  // A state transition should be made.
  assert(false);
}

/**
 * Calculates the time to spend in the next state.
 */ 
double DiseaseModel::getTimeInNextState(int nextState, std::default_random_engine generator) {
  loimos::proto::DiseaseModel_DiseaseState diseaseState =
      model->disease_state(nextState);
  if (diseaseState.has_fixed()) {
    return timeDefToSeconds(diseaseState.fixed().time_in_state());

  } else if (diseaseState.has_forever()) {
    return __INT_MAX__;

  } else if (diseaseState.has_uniform()) {
    double lower = timeDefToSeconds(diseaseState.uniform().tmin());
    double upper = timeDefToSeconds(diseaseState.uniform().tmax());
    std::uniform_real_distribution<double> uniform_dist(lower, upper);
    return uniform_dist(generator);
  }
  // TODO(iancostello): implement other strategies
  return 0;
}

double DiseaseModel::timeDefToSeconds(Time_Def time) {
  return time.hours() * 3600 + time.minutes() * 60;
}

int DiseaseModel::getTotalStates() {
  return model->disease_state_size();
}