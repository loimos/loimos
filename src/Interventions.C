#include "Interventions.h"
#include "readers/DataInterface.h"
#include "AttributeTable.h"
//#include "readers/DataReader.h"
#include <string>
#include <vector>

bool BaseIntervention::test(DataInterface& p,std::default_random_engine *generator)  {
  return true;
} 

void BaseIntervention::apply(DataInterface& p) {
  std::vector<union Data> &objData = p.getDataField();
  objData[2].probability = 0.99;
} 

void BaseIntervention::pup(PUP::er &p) {
  p | chance;
}

BaseIntervention::BaseIntervention(){
  this->chance = 1.0;
}

VaccinationIntervention::VaccinationIntervention(){
  this->chance = 1.0;
}

VaccinationIntervention::VaccinationIntervention(const AttributeTable& t){
  this->probIndex = t.getAttribute("prob");
  this->vaccinatedIndex = t.getAttribute("vaccinated");
  this->riskIndex = t.getAttribute("risk");
  this->chance = 0.5;
}

void VaccinationIntervention::pup(PUP::er &p) {
  p | probIndex;
  p | vaccinatedIndex;
  p | riskIndex;
  p | chance;
}

bool VaccinationIntervention::test(DataInterface& p, std::default_random_engine *generator)  {
  std::vector<union Data> &objData = p.getDataField();
  std::uniform_real_distribution<double> distribution(0.0,1.0);
  bool applied = distribution(*generator) < this->chance;
  //printf("Vaccinated:%d, riskProbability: %f, selected for vaccine %d\n", objData[vaccinatedIndex].boolean, objData[riskIndex].probability, applied);
  return !objData[vaccinatedIndex].boolean && objData[riskIndex].probability > 0.50 && applied;
} 

void VaccinationIntervention::apply(DataInterface& p)  {
  std::vector<union Data> &objData = p.getDataField();
  //printf("Before apply %f\n",objData[probIndex].probability);
  objData[vaccinatedIndex].boolean = true;
  objData[probIndex].probability = 0.85;
  //printf("After apply %f\n",objData[probIndex].probability);
} 



