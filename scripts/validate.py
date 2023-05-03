#!/usr/bin/env python3

import os
import numpy as np
import pandas as pd
import argparse

from utils.ids import partitioned_merge, init_multiprocessing


def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required args
    parser.add_argument(
        "input_dir",
        metavar="I",
        help="The path to the directory containing the input files",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-s",
        "--state",
        default=None,
        help="The abbreviated name of the state the data is for. If "
        + "specified, we also examine the intial activity assignment "
        + "and seperate activity and residence locaiton files",
    )
    parser.add_argument(
        "-n",
        "--num-tasks",
        default=1,
        type=int,
        help="Specifies the number of processes to use (default is serial)",
    )
    parser.add_argument(
        "-np",
        "--num-partitions",
        default=128,
        type=int,
        help="Specifies the number of partitions to seperate data into"
        + "before merging",
    )

    return parser.parse_args()


def main():
    # Parse args and read data
    args = parse_args()
    input_dir = args.input_dir
    num_tasks = args.num_tasks
    num_partitions = args.num_partitions

    print(f"Validating population at {input_dir}")

    # If a state is specified, we can also examine the initial activity
    # assignment and residence/activity location files
    if args.state is not None:
        state = args.state
        activity_location_assignment = pd.read_csv(
            os.path.join(
                input_dir, f"{state}_activity_location_assignment_week_final.csv"
            )
        )
        activity_locations = pd.read_csv(
            os.path.join(input_dir, f"{state}_activity_locations_final.csv")
        )
        residence_locations = pd.read_csv(
            os.path.join(input_dir, f"{state}_residence_locations_final.csv")
        )

        HOME_SHIFT = 1000000000
        activity_visits_mask = activity_location_assignment["lid"] < HOME_SHIFT
        residence_visits_mask = ~activity_visits_mask

        activity_visits = activity_location_assignment[activity_visits_mask]
        residence_visits = activity_location_assignment[residence_visits_mask]

        # This is here to potentially explain subsequent discrepancies,
        # but is not a hard check, since this data isn't ultimately used in
        # the run
        print("Make sure visits and location lid ranges line up:")
        print(
            "  Activity visits lid range: {} to {}".format(
                activity_visits["lid"].min(), activity_visits["lid"].max()
            )
        )
        print(
            "  Activity locations lid range: {} to {}".format(
                activity_locations["lid"].min(), activity_locations["lid"].max()
            )
        )
        print(
            "  Residence visits lid range: {} to {}".format(
                residence_visits["lid"].min(), residence_visits["lid"].max()
            )
        )
        print(
            "  Residence locations lid range: {} to {}".format(
                residence_locations["lid"].min(), residence_locations["lid"].max()
            )
        )

    people = pd.read_csv(os.path.join(input_dir, "people.csv"))
    locations = pd.read_csv(os.path.join(input_dir, "locations.csv"))
    visits = pd.read_csv(os.path.join(input_dir, "visits.csv"))

    print("Make sure ids are contiguous")
    assert np.all(people["pid"] == people.index)
    print("  People ids are contigous!")
    assert np.all(locations["lid"] == locations.index)
    print("  Locations ids are contigous!")

    print("Make sure all locations and people referenced in visits exist")
    init_multiprocessing()
    print(
        "  Visits pid range: {} to {}".format(visits["pid"].min(), visits["pid"].max())
    )
    print(
        "  People pid range: {} to {}".format(people["pid"].min(), people["pid"].max())
    )
    print(
        "  Visits lid range: {} to {}".format(visits["lid"].min(), visits["lid"].max())
    )
    print(
        "  Locations lid range: {} to {}".format(
            locations["lid"].min(), locations["lid"].max()
        )
    )
    visitors = partitioned_merge(
        people,
        visits,
        "pid",
        num_tasks=num_tasks,
        num_partitions=num_partitions,
        args={"how": "inner", "validate": "one_to_many"},
    )
    visited = partitioned_merge(
        locations,
        visits,
        "lid",
        num_tasks=num_tasks,
        num_partitions=num_partitions,
        args={"how": "inner", "validate": "one_to_many"},
    )
    print(f"  total visits: {visits.shape[0]}")
    print(f"  visits with valid pid: {visitors.shape[0]}")
    print(f"  visits with valid lid: {visited.shape[0]}")
    assert visitors.shape[0] == visits.shape[0]
    assert visited.shape[0] == visits.shape[0]

    print("All tests passed!")


if __name__ == "__main__":
    main()
