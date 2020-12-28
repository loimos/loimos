#include <vector>

#include "../DataReader.h"
#include "../DataInterface.h"
#include "../../Person.h"
#include "../data.pb.h"

#include <google/protobuf/text_format.h>

int main() {
    // Create 3 people that will all have 4 non-ignored attributes.
    std::vector<Person*> data;
    for (int p = 0; p < 3; p++) {
        data.push_back(new Person(4, 0, 1000));
    }

    // Read in input data format.
    loimos::proto::CSVDefinition personDef;
    std::ifstream t("persondef.textproto");
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    if (!google::protobuf::TextFormat::ParseFromString(str, &personDef)) {
        printf("Failed to read!\n");
        exit(1);
    }

    // Open CSV.
    std::ifstream file("persondata.csv");
    if (!file) {
        printf("failed to open.\n");
        exit(1);
    }

    // Open CSV
    DataReader<Person *>::readData(&file, &personDef, &data);

    for (Person *p : data) {
        p->_print_information(&personDef);
    }

    return 0;
}