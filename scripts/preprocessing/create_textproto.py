#!/usr/bin/env python3

import argparse
import os
import sys

import pandas as pd
from subprocess import check_output

PEOPLE_TYPES = {
    "pid": "unique_id",
    "age": "int32",
}
LOCATIONS_TYPES = {
    "lid": "unique_id",
    "max_simultaneous_visits": "int32",
    "school": "bool",
    "home": "bool",
}
VISITS_TYPES = {
    "pid": "unique_id",
    "lid": "foreign_id",
    "start_time": "start_time",
    "duration": "duration",
}
LOAD_COLUMNS = {"locations": "total_visits"}
DEFAULT_TYPE = "ignore"
COLUMN_METADATA_ENTRY = """fields {{
    field_name: \"{name}\"
    {dtype}: {{}}
}}
"""
TIMEDEF_ENTRY = """{name}: {{
    {unit}: {value}
}}
"""
SINGLETON_ENTRY = "{name}: {value}\n"


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
    parser.add_argument(
        "-m",
        "--visits-metadata",
        action="store_true",
        help="Set this flag to compute detailed metadata for visits"
    )
    parser.add_argument(
        "-P",
        "--print-only",
        action="store_true",
        help="Set this flag to write textproto files to stdout rather than "
        + "override the existing files"
    )

    return parser.parse_args()


# Based on response given in:
# https://stackoverflow.com/questions/6520761/running-wc-l-filename-within-python-code
def wc(path):
    result = check_output(["wc", "-l", path])
    return int(result.split()[0])


def get_basic_metadata(csv_path):
    # Ignore header row
    num_rows = wc(csv_path) - 1
    return [SINGLETON_ENTRY.format(name="num_rows", value=num_rows)]


def get_visits_metadata(csv_path, day_length=86400):
    visits = pd.read_csv(csv_path)

    start_time = visits["start_time"].min()
    end_time = (visits["start_time"] + visits["duration"]).max()

    start_day = start_time // day_length
    end_day = end_time // day_length

    return [
        SINGLETON_ENTRY.format(name="num_rows", value=visits.shape[0]),
        TIMEDEF_ENTRY.format(name="start_time", unit="days",
            value=start_day),
        TIMEDEF_ENTRY.format(name="duration", unit="days",
            value=end_day - start_day),
    ]


GET_METADATA = {
    "basic": get_basic_metadata,
    "visits": get_visits_metadata,
}
def create_textproto(pop_dir, csv_filename, dtypes, partition_offsets=None,
        metadata_type="basic", print_only=False):
    csv_path = os.path.join(pop_dir, csv_filename)
    tmp, _ = os.path.splitext(csv_path)
    textproto_path = tmp + ".textproto"
    name = os.path.basename(tmp)

    metadata = GET_METADATA[metadata_type](csv_path)
    df = pd.read_csv(csv_path, nrows=1)

    def write_entries(f):
        for entry in metadata:
            f.write(entry)
        if name in LOAD_COLUMNS:
            f.write(
                SINGLETON_ENTRY.format(
                    name="load_column", value=f'"{LOAD_COLUMNS[name]}"'
                )
            )

        if partition_offsets is not None:
            for offset in partition_offsets:
                f.write(
                    SINGLETON_ENTRY.format(name="partition_offsets", value=f"{offset}")
                )

        for c in df.columns:
            dtype = dtypes.get(c, DEFAULT_TYPE)
            f.write(COLUMN_METADATA_ENTRY.format(name=c, dtype=dtype))

    if print_only:
        print(f"{name}.textproto:")
        write_entries(sys.stdout)
        print("")
        print("")
    else:
        with open(textproto_path, "w") as f:
            write_entries(f)


def main():
    args = parse_args()

    visits_metadata_type = "basic"
    if args.visits_metadata:
        visits_metadata_type = "visits"

    create_textproto(args.pop_dir, args.people_file, PEOPLE_TYPES,
            print_only=args.print_only)
    create_textproto(args.pop_dir, args.locations_file, LOCATIONS_TYPES,
            print_only=args.print_only)
    create_textproto(args.pop_dir, args.visits_file, VISITS_TYPES,
            metadata_type=visits_metadata_type, print_only=args.print_only)


if __name__ == "__main__":
    main()
