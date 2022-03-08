/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AGGREGATOR_H__
#define __AGGREGATOR_H__

#include "AggregatorParam.h"
#include "Message.h"

#include <hypercomm/routing.hpp>
#include <hypercomm/aggregation.hpp>
#include <memory>

using buffer_t = aggregation::direct_buffer;
using routing_t = aggregation::routing::mesh<2>;
using visit_aggregator_t = aggregation::array_aggregator<buffer_t, routing_t, VisitMessage>;
using interact_aggregator_t = aggregation::array_aggregator<buffer_t, routing_t, InteractionMessage>;

class Aggregator : public CBase_Aggregator {
  public:
    std::shared_ptr<visit_aggregator_t> visit_aggregator;
    std::shared_ptr<interact_aggregator_t> interact_aggregator;

    Aggregator(AggregatorParam p1, AggregatorParam p2);
};

#endif // __AGGREGATOR_H__
