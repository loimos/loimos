/* Copyright 2020 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PEOPLE_H__
#define __PEOPLE_H__

class People : public CBase_People {
  public:
    People();
    void SendVisitMessages(); // calls ReceiveVisitMessages
    void UpdateDiseaseState();
    void ReportStats();
};

#endif // __PEOPLE_H__
