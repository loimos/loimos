/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __DiseaseModel_H__
#define __DiseaseModel_H__

#include "disease.pb.h"
#include "distribution.pb.h"

class DiseaseModel : public CBase_DiseaseModel {
    private:
        loimos::proto::DiseaseModel *model;
        // Map from state name to index of state in model.
        std::map<std::string, int> *state_lookup;
        // For each state index, map from stategy name string to index of strategy labels.
        std::vector<std::map<std::string, int> *> *stategy_lookup;  
        int uninfectedState;
        double getTimeInNextState(int nextState, std::default_random_engine generator);
        double timeDefToSeconds(Time_Def time);
    public:
        DiseaseModel(std::string pathToModel);
        int getIndexOfState(std::string stateLabel);
        std::tuple<int, int> transitionFromState(int fromState, std::string interventionStategy, std::default_random_engine generator);
        int getTotalStates();
        std::string lookupStateName(int state);
};

#endif // __DiseaseModel_H__ 