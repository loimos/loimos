/* Copyright 2020-2023 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __INTERACTION_H__
#define __INTERACTION_H__

// Simple struct to hold data on an interfaction between with a susceptible
// person which could lead to an infection
struct Interaction {
  // Describes the chance of this interaction resulting in an infection
  double propensity;
  // Data on the person who could potentially infect the susceptible person in
  // question
  int infectiousIdx;
  int infectiousState;
  // The state the susceptible person will transition to if this results in
  // a infection
  // TODO: figure out how to extract these from an infectious/susceptible
  // state pair
  //int targetState;
  // We need to know when the interaction occured so that, if this interaction
  // does in fact result in an infection, we can determine precisely when it
  // occured
  int startTime;
  int endTime;

  // Lets us send potential infections via charm++
  void pup(PUP::er &p);

  // We need this in order to emplace Interactions...
  Interaction(
    double propensity,
    int infectiousIdx,
    int infectiousState,
    int startTime,
    int endTime
  );
  // ...and we need to define these explicitly since we added a constructor
  Interaction() = default;
  Interaction(const Interaction&) = default;
  Interaction(Interaction&&) = default;
  Interaction &operator = (const Interaction&) = default;
  Interaction &operator = (Interaction&&) = default;
};

#endif // __INTERACTION_H__
