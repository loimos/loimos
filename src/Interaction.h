/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef INTERACTION_H_
#define INTERACTION_H_

// Simple struct to hold data on an interfaction between with a susceptible
// person which could lead to an infection
struct Interaction {
  // Describes the chance of this interaction resulting in an infection
  double propensity;
  // Data on the person who could potentially infect the susceptible person in
  // question
  int infectiousIdx;
  int infectiousState;
  // We need to know when the interaction occured so that, if this interaction
  // does in fact result in an infection, we can determine precisely when it
  // occurred
  int startTime;
  int endTime;

  // Lets us send potential infections via charm++
  void pup(PUP::er &p);  // NOLINT(runtime/references)

  // We need this in order to emplace Interactions...
  Interaction(
    double propensity,
    int infectiousIdx,
    int infectiousState,
    int startTime,
    int endTime);
  // ...and we need to define these explicitly since we added a constructor
  Interaction() = default;
  Interaction(const Interaction&) = default;
  Interaction(Interaction&&) = default;
  Interaction &operator = (const Interaction&) = default;
  Interaction &operator = (Interaction&&) = default;
};

#endif  // INTERACTION_H_
