/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXTERN_H_
#define EXTERN_H_

#include "loimos.decl.h"

#include <string>

extern /* readonly */ CProxy_Main mainProxy;
extern /* readonly */ CProxy_People peopleArray;
extern /* readonly */ CProxy_Locations locationsArray;
#ifdef USE_HYPERCOMM
extern /* readonly */ CProxy_Aggregator aggregatorProxy;
#endif
extern /* readonly */ CProxy_Scenario globScenario;

extern /* readonly */ Counter totalVisits;
extern /* readonly */ Counter totalInteractions;
extern /* readonly */ Counter totalExposures;
extern /* readonly */ Counter totalExposureDuration;
extern /* readonly */ double simulationStartTime;
extern /* readonly */ double iterationStartTime;

#endif  // EXTERN_H_
