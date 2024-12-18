#!/usr/bin/env python3
import argparse
import os
import re

import pandas as pd
from scipy.stats import wasserstein_distance


def parse_args():
    print("""
          Script takes all results.csv files renamed in the 
          form [input_dir]/{series_name}/{datapoint_name}_{trial_number}.csv 
          and calculates average earthmover distance to 
          control distribution [control_csv], 
          outputting [output_dir]/averages.csv, [output_dir]/data.csv
          """)

    parser = argparse.ArgumentParser()

    # Positional/required arguments:
    parser.add_argument(
        "input_dir",
        metavar="I",
        help="The path to directory containing data files for a population",
    )
    parser.add_argument(
        "output_dir",
        metavar="O",
        help="The directory for the output averages and data csv files",
    )

    parser.add_argument(
        "control_csv",
        metavar="C",
        help="CSV against which to compare to produce earthmover distances",
    )
    return parser.parse_args()


def compute_earthmover_distance(input_dir, control_file, k):
    control = pd.read_csv(control_file, usecols=["infectious_count"]).iloc[k].values

    all_datapoints_bucketed = {}
    all_datapoints = {}

    for root, dirs, files in os.walk(input_dir):
        for series_directory in dirs:
            series_path = os.path.join(root, series_directory)
            for file in os.listdir(series_path):
                if not file.endswith(".csv"):
                    print(f"file {file} is not a csv")
                    continue
                input = os.path.join(series_path, file)
                try:
                    current = pd.read_csv(input, usecols=["infectious_count"]).iloc[k].values
                except IndexError:
                    print(f"file {file} does not have a row {k}")
                    continue

                print(f"calculating earthmover distance for {file}")
                datapoint = re.compile(r"(.+)_\d+.csv").match(file).group(1)
                if datapoint is None:
                    raise ValueError(f"{file}: datapoint not found in file name; should be in form [datapoint]_[trial_number].csv")

                if datapoint not in all_datapoints_bucketed:
                    all_datapoints_bucketed[datapoint] = []

                all_datapoints_bucketed[datapoint].append(current)

    for datapoint, values in all_datapoints_bucketed.items():
        if len(values) == 0:
            all_datapoints[datapoint] = 0
        else:
            concatenated_values = pd.concat([pd.Series(v) for v in values], axis=0).values
            distance = wasserstein_distance(control, concatenated_values)
            all_datapoints[datapoint] = distance

    return all_datapoints


def main():
    args = parse_args()

    print("checking input validity")
    if not os.path.exists(args.input_dir):
        print(f"input directory not found: {args.input_dir}")
        raise FileNotFoundError(args.input_dir)

    if len(os.listdir(args.input_dir)) == 0:
        print("input directory is empty")
        raise FileNotFoundError(args.input_dir)

    csvcols = ["day", "infectious_count"]
    control = pd.read_csv(args.control_csv, usecols=csvcols)["infectious_count"]

    all_datapoints = {}
    datapoint_averages_by_series = {}

    for i in range(200):
        print(compute_earthmover_distance(args.input_dir, args.control_csv, i))

    averages = pd.DataFrame(datapoint_averages_by_series)
    averages.to_csv(f"{args.output_dir}/averages.csv")

    data = pd.DataFrame(all_datapoints)
    data.to_csv(f"{args.output_dir}/data.csv")
            


if __name__ == "__main__":
    main()
