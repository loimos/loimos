#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <tuple>
#include <string>

std::tuple<int, int, std::string> build_cache(std::string scenarioPath, int numPeople, int peopleChares, int numLocations, int numLocationChares, int numDays);

#endif