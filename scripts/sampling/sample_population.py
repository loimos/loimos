#!/usr/bin/env python3

import argparse
import os
import pandas as pd

def parse_args():
    parser = argparse.ArgumentParser(
        description="Samples a given set of locations (along with their "
        + "associated visits and people) from the given population"
    )

    # Positional/required arguments:
    parser.add_argument(
        "population_dir",
        metavar="P",
        help="The path to directory containing data files for a population",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-l",
        "--locations",
        nargs="+",
        default=None,
        help="A list of location ids to sample"
    )
    parser.add_argument(
        "-lp",
        "--locations-file",
        nargs="+",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir"
    )
    parser.add_argument(
        "-pp",
        "--people-file",
        nargs="+",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir"
    )
    parser.add_argument(
        "-vp",
        "--visits-file",
        nargs="+",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir"
    )

    return parser.parse_args()

def main():
    args = parse_args()

    in_dir = args.population_dir
    locations_file = os.path.join(in_dir, args.locations_file)
    people_file = os.path.join(in_dir, args.people_file)
    visits_file = os.path.join(in_dir, args.visits_file)

    locations = pd.read_csv(locations_file)
    people = pd.read_csv(people_file)
    visits = pd.read_csv(visits_file)

    if args.locations is None:
        busiest_lid = locations["max_simultaneous_visits"].argmax()
        print(locations.iloc[busiest_lid])


if __name__ == '__main__':
    main()
