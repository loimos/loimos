/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __DiseaseModel_H__
#define __DiseaseModel_H__

#include "Event.h"

#include "disease_model/disease.pb.h"
#include "disease_model/distribution.pb.h"

#include "Event.h"
 #include "readers/data.pb.h"

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
        Time getTimeInNextState(int nextState, std::default_random_engine *generator) const;
        Time timeDefToSeconds(Time_Def time) const;
        int healthyState;
    public:
        DiseaseModel(std::string pathToModel);
        int getIndexOfState(std::string stateLabel) const;
        // TODO(iancostello): Change interventionStategies to index based.
        std::tuple<int, int> transitionFromState(int fromState, std::string interventionStategy, std::default_random_engine *generator) const;
        std::string lookupStateName(int state) const;
        int getNumberOfStates() const;
        int getHealthyState() const;
        bool isInfectious(int personState) const;
        bool isSusceptible(int personState) const;
        const char * getStateLabel(int personState) const;
        double getLogProbNotInfected(Event susceptibleEvent, Event infectiousEvent) const;
        double getPropensity(
          int susceptibleState,
          int infectiousState,
          int startTime,
          int endTime
        ) const;

        // These objects are not related to the disease model but are
        // per PE definitions so it makes sense to share them.
        loimos::proto::CSVDefinition *personDef;
        loimos::proto::CSVDefinition *locationDef;
        loimos::proto::CSVDefinition *activityDef;
};

#endif // __DiseaseModel_H__
