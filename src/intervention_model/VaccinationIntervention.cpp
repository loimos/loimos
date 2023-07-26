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
    const loimos::proto::DiseaseModel &diseaseDef,
    const AttributeTable &t) :
  Intervention(interventionDef, diseaseDef, t) {
  vaccinationProbability = interventionDef.vaccination().probability();
  vaccinatedSusceptibility = interventionDef.vaccination()
    .vaccinated_susceptibility();
  // CkPrintf("Vaccination: prob: %f, susceptibility: %f\n",
  //     vaccinationProbability, vaccinatedSusceptibility);
  this->vaccinatedIndex = t.getAttributeIndex("vaccinated");
  this->susceptibilityIndex = t.getAttributeIndex("susceptibility");
}

bool VaccinationIntervention::test(const Person &p,
    std::default_random_engine *generator) const {
  return !p.getValue(vaccinatedIndex).boolean
    && unitDistrib(*generator) < vaccinationProbability;
}

void VaccinationIntervention::apply(Person *p) const {
  std::vector<union Data> &objData = p->getData();
  objData[vaccinatedIndex].boolean = true;
  objData[susceptibilityIndex].double_b10 = vaccinatedSusceptibility;
}
