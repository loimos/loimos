 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "Interventions.h"
#include "readers/DataInterface.h"
#include "AttributeTable.h"

#include <vector>

void BaseIntervention::pup(PUP::er &p) {}

bool BaseIntervention::test(const DataInterface &p,
    std::default_random_engine *generator) {
  return false;
}

void BaseIntervention::apply(DataInterface *p) {}

VaccinationIntervention::VaccinationIntervention(const AttributeTable &t) {
  vaccinationProbability = 0.5;
  this->probIndex = t.getAttribute("prob");
  this->vaccinatedIndex = t.getAttribute("vaccinated");
  this->riskIndex = t.getAttribute("risk");
}

void VaccinationIntervention::pup(PUP::er &p) {
  p | vaccinationProbability;
  p | probIndex;
  p | vaccinatedIndex;
  p | riskIndex;
}

bool VaccinationIntervention::test(const DataInterface &p,
    std::default_random_engine *generator) {
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  // CkPrintf("Vaccinated:%d, riskProbability: %f, selected for vaccine %d\n",
  //     p.getValue(vaccinatedIndex).boolean, p.getValue(riskIndex).probability, applied);
  return !p.getValue(vaccinatedIndex).boolean
    && distribution(*generator) < vaccinationProbability;
}

void VaccinationIntervention::apply(DataInterface *p)  {
  std::vector<union Data> &objData = p->getData();
  // CkPrintf("Before apply %f\n",objData[probIndex].probability);
  objData[vaccinatedIndex].boolean = true;
  objData[probIndex].probability = 0.85;
  // CkPrintf("After apply %f\n",objData[probIndex].probability);
}
