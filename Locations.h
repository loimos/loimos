/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOCATIONS_H__
#define __LOCATIONS_H__

class Locations : public CBase_Locations {
  public:
    Locations();
    void ReceiveVisitMessages();
    void ComputeInteractions(); // calls UpdateDiseaseState
};

#endif // __LOCATIONS_H__
