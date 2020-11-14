#include "loimos.decl.h"
#include "Defs.h"

#include <string>

int getIntAttribute(std::string *str, int index) {
    int attr_index = 0;
    int left_comma = 0;
    for (int c = 0; c < str->length(); c++) {
        if (str->at(c) == ',') {
        // Completed attribute.
        std::string sub_str = str->substr(left_comma, c);

        //TODO HANDLE THESE ATTRIBUTES WITH PROTOBUF
        if (attr_index == index) {
            try {
                return std::stoi(sub_str);
            } catch (const std::exception& e) {
                CkPrintf("Could not parse %s\n", sub_str.c_str());
                return 0;
            }
        }

        left_comma = c + 1;
        attr_index += 1;
        }
  }
}