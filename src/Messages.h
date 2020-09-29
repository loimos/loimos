/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

struct VisitMessage {
  int personIdx;
  char personState;
  int locationIdx;

  VisitMessage() {}
  VisitMessage(int personIdx_, char personState_, int locationIdx_) :
    personIdx(personIdx_), personState(personState_), locationIdx(locationIdx_) {}
};
PUPbytes(VisitMessage);

#endif // __MESSAGES_H__
