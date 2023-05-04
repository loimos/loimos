/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#include "loimos.decl.h"
#include "Defs.h"
#include "Aggregator.h"

Aggregator::Aggregator(AggregatorParam p1, AggregatorParam p2) {
  constexpr auto cond = CcdPERIODIC;  // Raised every few milliseconds

  if (p1.useAggregator) {
    if (CkMyPe() == 0) {
      CkPrintf("Creating VisitMessage aggregator with buffer size %lu,"
          " threshold %.6lf, flush period %.6lf, node-level %d\n",
          p1.bufferSize, p1.threshold, p1.flushPeriod,
          static_cast<int>(p1.nodeLevel));
    }
    visit_aggregator = std::make_shared<visit_aggregator_t>(
        locationsArray, CkIndex_Locations::ReceiveVisitMessages(VisitMessage{}),
        p1.bufferSize, p1.threshold, p1.flushPeriod, p1.nodeLevel, cond);
  } else {
    if (CkMyPe() == 0) {
      CkPrintf("Not using VisitMessage aggregator\n");
    }
    visit_aggregator = nullptr;
  }

  if (p2.useAggregator) {
    if (CkMyPe() == 0) {
      CkPrintf("Creating InteractionMessage aggregator with buffer size %lu,"
          " threshold %.6lf, flush period %.6lf, node-level %d\n",
          p2.bufferSize, p2.threshold, p2.flushPeriod,
          static_cast<int>(p2.nodeLevel));
    }
    interact_aggregator = std::make_shared<interact_aggregator_t>(
        peopleArray, CkIndex_People::ReceiveInteractions(InteractionMessage{}),
        p2.bufferSize, p2.threshold, p2.flushPeriod, p2.nodeLevel, cond);
  } else {
    if (CkMyPe() == 0) {
      CkPrintf("Not using InteractionMessage aggregator\n");
    }
    interact_aggregator = nullptr;
  }

  // Notify Main
  contribute(CkCallback(CkReductionTarget(Main, CharesCreated), mainProxy));
}
