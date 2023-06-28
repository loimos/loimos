 /* Copyright 2020-2023 The Loimos Project Developers.
  * See the top-level LICENSE file for details.
  *
  * SPDX-License-Identifier: MIT
  */

#include "Intervention.h"
#include "VaccinationIntervention.h"
#include "AttributeTable.h"
#include "../protobuf/interventions.pb.h"
#include "../readers/DataInterface.h"

#include <vector>

VaccinationIntervention::VaccinationIntervention(
    const loimos::proto::InterventionModel::Intervention &interventionDef,
    const AttributeTable &t) {
  vaccinationProbability = interventionDef.vaccination().probability();
  vaccinatedSusceptibility = interventionDef.vaccination()
    .vaccinated_susceptibility();
  CkPrintf("Vaccination: prob: %f, susceptibility: %f\n",
      vaccinationProbability, vaccinatedSusceptibility);
  this->vaccinatedIndex = t.getAttribute("vaccinated");
  this->susceptibilityIndex = t.getAttribute("susceptibility");
}

void VaccinationIntervention::pup(PUP::er &p) {
  p | vaccinationProbability;
  p | vaccinatedSusceptibility;
  p | vaccinatedIndex;
  p | susceptibilityIndex;
}

bool VaccinationIntervention::test(const DataInterface &p,
    std::default_random_engine *generator) const {

  return !p.getValue(vaccinatedIndex).boolean
    && unitDistrib(*generator) < vaccinationProbability;
}

void VaccinationIntervention::apply(DataInterface *p) const {
  std::vector<union Data> &objData = p->getData();
  objData[vaccinatedIndex].boolean = true;
  objData[susceptibilityIndex].double_b10 = vaccinatedSusceptibility;
}
