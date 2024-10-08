#!/usr/bin/env python3
# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

import pandas as pd
import numpy as np
import argparse
import os
import sys
import time
import functools

from multiprocessing import Pool, set_start_method

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils.shared import SharedPandasDataFrame  # noqa

"""
Given a visit schedule, this script calculates the total visits and maximum
simultaneous visits for all locations in the dataset.
"""

# Column name that describes the location id being visited.
LID_COL = "lid"
# Any other column in the dataset that is fully populated.
START_COL = "start_time"


#test_set = {2,3,4}
def find_max_simultaneous_visits(lid, visits):
    events = visits.melt(
        value_vars=["start_time", "end_time"], value_name="time", var_name="type"
    )
    events.sort_values(["time", "type"], inplace=True)
    events["occupancy"] = -1
    events.loc[events["type"] == "start_time", "occupancy"] = 1
    result = events["occupancy"].cumsum().max()
    #if lid in test_set:
    #   print(f"location {lid}: {visits.shape[0]} visits, {result} msv")
    #   print(events)
    #   # print(visits.memory_usage())
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Calculates summary statistics for a given visit file."
    )

    # Positional/required arguments:
    parser.add_argument(
        "population_dir",
        metavar="P",
        help="The path to directory containing data files for a population",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-v",
        "--visits-file",
        default="visits.csv",
        help="The name of the visits file within population_dir. Should be "
        + "a csv file with a column called 'lid'",
    )
    parser.add_argument(
        "-l",
        "--locations-file",
        default="locations.csv",
        help="The name of the locations file within population_dir. Should "
        + "a csv file with a column called 'lid'",
    )
    parser.add_argument(
        "-o",
        "--output-file",
        default="{name}.heuristic.csv",
        help="Format of output files ('{name}' will be replaced by the "
        + "name of the corresponding input file, ignoring the extension)."
        + "This output will be a csv containing the data from the "
        + "locations file, plus some extra information about visits "
        + "computed by this script.",
    )
    parser.add_argument(
        "-O",
        "--override",
        action="store_true",
        help="Pass this argument to overwrite the locations file with the "
        + "output, rather than saving it to the specified output file",
    )
    parser.add_argument(
        "-n",
        "--n-tasks",
        default=1,
        type=int,
        help="Specifies the number of processes to use (default is serial)",
    )
    args = parser.parse_args()

    # Get command line arguments, and place input files in the population_dir
    path_to_visits = os.path.join(args.population_dir, args.visits_file)
    path_to_locations = os.path.join(args.population_dir, args.locations_file)

    # Output file defaults to given path with .heuristic.csv file extension.
    output_file = args.output_file.format(
        name=path_to_locations[: path_to_locations.rfind(".")]
    )
    # If the override flag is set, save output to locations_file instead
    if args.override:
        output_file = path_to_locations

    # Load dataset.
    print(f"Loading visits from {path_to_visits}")
    visits = pd.read_csv(path_to_visits)
    visits["end_time"] = visits["start_time"] + visits["duration"]

    # Calculate total visits to a location.
    start_time = time.perf_counter()
    visits_by_location = visits[[LID_COL, "start_time", "end_time"]].groupby(LID_COL)
    max_visits = pd.DataFrame(
        visits_by_location["start_time"].count().rename("total_visits")
    )
    end_time = time.perf_counter()
    print("Calculating total visits:", end_time - start_time)

    renaming = []
    # if "daynum" in visits:
    #    # Calculate total visits for each day per location.
    #    start_time = time.perf_counter()
    #    daily_summaries = (
    #        visits[["lid", "daynum", "start_time"]]
    #        .groupby(["lid", "daynum"])
    #        .count()
    #        .unstack()
    #        .fillna(0)
    #    )
    #    # Rename columns.
    #    for column in daily_summaries.columns:
    #        renaming.append(f"total_on_day_{column[1]}")
    #    daily_summaries.columns = renaming

    #    # Calculate some additional statistics.
    #    daily_summaries["average_daily_total"] = daily_summaries.mean(axis=1)
    #    daily_summaries["median_daily_total"] = daily_summaries.median(axis=1)
    #    daily_summaries["max_daily_total"] = daily_summaries.max(axis=1)
    #    # Merge in.
    #    max_visits = max_visits.merge(
    #        daily_summaries, left_index=True, right_index=True
    #    )

    #    end_time = time.perf_counter()
    #    print("Calculating daily summaries:", end_time - start_time)

    # Calculate the maximum simulatenous visits using as many processes
    # as possible
    start_time = time.perf_counter()
    # print(visits.memory_usage())
    # max_visits = pd.DataFrame(max_visits)
    if args.n_tasks > 1:
        # The default way of starting new processes - fork - duplicates the
        # entire process - including its memory footprint - so let's choose
        # another method (see https://stackoverflow.com/questions/42584525/
        # python-multiprocessing-debugging-oserror-errno-12-cannot-allocate-memory
        set_start_method("spawn")
        # print(visits_by_location.groups, flush=True)
        #shared_visits = SharedPandasDataFrame(visits)

        with Pool(args.n_tasks) as pool:
            # visits_by_location = {k: SharedPandasDataFrame(df) \
            #        for k, df in visits_by_location.groups.items()}
            max_visits["max_simultaneous_visits"] = pool.starmap(
                find_max_simultaneous_visits,
                visits_by_location,
            )

        #shared_visits.unlink()
    else:
        max_visits["max_simultaneous_visits"] = [
            find_max_simultaneous_visits(lid, group)
            for lid, group in visits_by_location
        ]

    end_time = time.perf_counter()
    print("Calculating maximum simultaneous visits:", end_time - start_time)

    # print(max_visits.columns)
    # print(max_visits.index)

    # We need the max visit data to be a location attribute, so combine it
    # with the location data
    locations = pd.read_csv(path_to_locations)
    # If we've already calculated these heuristics before, we'll need to
    # overwite the old values, so let's get rid of them before we merge the
    # data
    overlap = set(np.intersect1d(locations.columns, max_visits.columns))
    locations.drop(axis="columns", labels=overlap - {LID_COL}, inplace=True)
    output_df = locations.merge(max_visits, how="left", on=LID_COL)
    # print(output_df)
    # print(output_df.columns)

    # Zero out the heuristic values for any location with no visits
    output_df.fillna(0, inplace=True)

    # Fix types of heuristic coluns so that we can read in integer attributes
    # properly in loimos
    output_dtypes = {
        "max_simultaneous_visits": int,
        "total_visits": int,
    }
    if "daynum" in visits:
        output_dtypes.update({"median_daily_total": int, "max_daily_total": int})
    output_dtypes.update({c: int for c in output_df.columns if "pub" in c})
    output_dtypes.update({c: int for c in renaming})
    output_df = output_df.astype(output_dtypes)

    # Output with index column which is the lids.
    print(f"Saving heuristics to {output_file}")
    output_df.to_csv(output_file, index=False)
