#!/usr/bin/env python3

import argparse
import os

import pandas as pd

PEOPLE_TYPES = {
    "pid": "unique_id",
    "age": "b10int",
}
LOCATIONS_TYPES = {
    "lid": "unique_id",
    "max_simultaneous_visits": "b10int",
    "school": "bool",
}
VISITS_TYPES = {
    "pid": "unique_id",
    "lid": "foreign_id",
    "start_time": "start_time",
    "duration": "duration",
}
DEFAULT_TYPE = "ignore"
TEXTPROTO_ENTRY = """fields {{
    field_name: \"{name}\"
    {dtype}: {{}}
}}
"""


def parse_args():
    parser = argparse.ArgumentParser(
        description="Samples a given set of locations (along with their "
        + "associated visits and people) from the given population"
    )

    # Positional/required arguments:
    parser.add_argument(
        "pop_dir",
        metavar="I",
        help="A path to a directory containing data on the population to sample from",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-p",
        "--people-file",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-l",
        "--locations-file",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-v",
        "--visits-file",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )

    return parser.parse_args()


def create_textproto(pop_dir, csv_filename, dtypes):
    csv_path = os.path.join(pop_dir, csv_filename)
    tmp, _ = os.path.splitext(csv_path)
    textproto_path = tmp + ".textproto"

    df = pd.read_csv(csv_path, nrows=1)
    with open(textproto_path, "w") as f:
        for c in df.columns:
            dtype = dtypes.get(c, DEFAULT_TYPE)
            f.write(TEXTPROTO_ENTRY.format(name=c, dtype=dtype))


def main():
    args = parse_args()

    create_textproto(args.pop_dir, args.people_file, PEOPLE_TYPES)
    create_textproto(args.pop_dir, args.locations_file, LOCATIONS_TYPES)
    create_textproto(args.pop_dir, args.visits_file, VISITS_TYPES)


if __name__ == "__main__":
    main()
