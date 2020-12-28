#include <string>
#include <iostream>
#include <fstream>
#include <vector>

int getIntAttribute(std::string *str, int index);
int getDay(int time_in_seconds);

/** 
 * This file preprocesses a given input file.
 */ 
// TODO change this.
int main(int argc, char **argv) {
    /**
     * Assumptions. 
     * Stream is sorted by start time per person.
     */ 
    std::ifstream activity_stream("sample_input/interactions.csv", std::ios_base::binary);
    if (!activity_stream) {
        printf("Could not open person data input.\n");
        exit(0);
    }

    // TOOD make this cmd line args.
    int id_location = 1;
    int start_time = 4;
    int start_id = 5586585;
    // int num_people = 40;
    int num_people = 41120;

    std::vector< std::vector<uint32_t> > elements;
    elements.resize(num_people);

    std::string line;
    uint32_t current_position = activity_stream.tellg();
    std::getline(activity_stream, line);
    int last_person = -1;
    int last_time = -1;
    int num_processed = 0;
    // printf("got here\n");

    while (!activity_stream.eof()) {
        // Skip until new day or new person.
        int next_person = 0;
        int next_time = 0;
        do {
            current_position = activity_stream.tellg();
            std::getline(activity_stream, line);
            next_person = getIntAttribute(&line, id_location);
            next_time = getDay(getIntAttribute(&line, start_time));
            
        } while (!activity_stream.eof() && last_time == next_time && last_person == next_person);

        // printf("got here %d\n", next_person);
        if (activity_stream.eof()) {
            break;
        }
        num_processed += 1;

        std::vector<uint32_t> *curr = &elements[next_person - start_id];
        for (int i = next_time; i < curr->size(); i++) {
            curr->push_back(0xFFFFFFFF);
        }
        // Add current day
        curr->push_back(current_position);

        // Update
        last_person = next_person;
        last_time = next_time;
    }

    // Output
    printf("Completed %d!\n", num_processed);
    std::ofstream output_stream("data_index_activity.csv");
    for (int i = 0; i < elements.size(); i++) {
        std::vector<uint32_t> curr = elements[i];
        if (curr.size() != 0) {
            output_stream << start_id + i << ",";
            for (int j = 0; j < curr.size(); j++) {
                output_stream << curr[j] << " ";
            }
            if (i != elements.size() - 1)
                output_stream << "\n";
        }
    }
    output_stream.close();

}

int getDay(int time_in_seconds) {
    return (time_in_seconds / (3600*24));
}

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
                printf("Could not parse %s\n", sub_str.c_str());
                return 0;
            }
        }

        left_comma = c + 1;
        attr_index += 1;
        }
  }
  return 0;
}