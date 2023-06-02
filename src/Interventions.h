#ifndef __INTERVENTIONS_H__
#define __INTERVENTIONS_H__

#include "readers/DataInterface.h"
#include "AttributeTable.h"
#include <sys/time.h>
//#include "readers/DataReader.h"
#include <string>
#include <vector>

class BaseIntervention : public PUP::able{
    
  public:
    virtual bool test(DataInterface& p, std::default_random_engine *generator);
    virtual void apply(DataInterface& p) ;
    virtual void pup(PUP::er &p);
    double chance;
    //std::default_random_engine generator;
    
    PUPable_decl(BaseIntervention);
    BaseIntervention();
    BaseIntervention(CkMigrateMessage *m) : PUP::able(m) {}
};


//include probability field
//include check for vaccination related field in attribute vector
class VaccinationIntervention : public BaseIntervention {
  public:
    int riskIndex;
    int vaccinatedIndex;
    int probIndex;
    void pup(PUP::er &p);
    bool test(DataInterface& p, std::default_random_engine *generator);
    void apply(DataInterface& p);
    void identify();
    VaccinationIntervention(const AttributeTable& t);
    PUPable_decl(VaccinationIntervention);
    VaccinationIntervention();
    VaccinationIntervention(CkMigrateMessage *m) : BaseIntervention(m) {}

};
#endif
