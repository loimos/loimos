/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __DiseaseModel_H__
#define __DiseaseModel_H__

#include "disease_model/disease.pb.h"
#include "disease_model/distribution.pb.h"

#include <random>

using Time = int32_t;

class DiseaseModel : public CBase_DiseaseModel {
    private:
        loimos::proto::DiseaseModel *model;
        // Map from state name to index of state in model.
        // TODO(iancostello): Change these maps to be non-pointers.
        std::unordered_map<std::string, int> *state_lookup;
        // For each state index, map from stategy name string to index of strategy labels.
        std::vector<std::unordered_map<std::string, int> *> *strategy_lookup;  
        Time getTimeInNextState(int nextState, std::default_random_engine *generator);
        Time timeDefToSeconds(Time_Def time);
        int healthyState;
    public:
        DiseaseModel(std::string pathToModel);
        int getIndexOfState(std::string stateLabel);
        // TODO(iancostello): Change interventionStategies to index based.
        std::tuple<int, int> transitionFromState(int fromState, std::string interventionStategy, std::default_random_engine *generator);
        std::string lookupStateName(int state);
        int getNumberOfStates();
        int getHealthyState();
        bool isInfectious(int personState);
};

#endif // __DiseaseModel_H__ 
