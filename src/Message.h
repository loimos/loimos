/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

struct VisitMessage {
  int locationIdx;
  int personIdx;
  int personState;
  int visitStart;
  int visitEnd;

  VisitMessage() {}
  VisitMessage(int locationIdx_, int personIdx_, int personState_, int visitStart_,
      int visitEnd_) : locationIdx(locationIdx_), personIdx(personIdx_),
      personState(personState_), visitStart(visitStart_), visitEnd(visitEnd_) {}
};
PUPbytes(VisitMessage);

#endif // __MESSAGE_H__
