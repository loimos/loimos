#!/usr/bin/env python3
import argparse
import os
import re

import pandas as pd
from scipy.stats import wasserstein_distance
from rich.progress import track


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
                    #print(f"file {file} does not have a row {k}")
                    current = 0
                except Exception as e:
                    print(f"exception processing file {file}: {e}")
                    continue

                datapoint = re.compile(r"(.+)_\d+.csv").match(file).group(1)
                #print(f"+{file} \t -> \t\t{datapoint}")

                if datapoint not in all_datapoints_bucketed:
                    all_datapoints_bucketed[datapoint] = []

                all_datapoints_bucketed[datapoint].append(current)

    # all_datapoints_bucketed contains all the various infectious count values for a given series (at the time step we're evaluating for)
    for datapoint, values in all_datapoints_bucketed.items():
        if len(values) == 0:
            all_datapoints[datapoint] = 0
        else:
            if len(values) < max_count:
                for i in range(max_count - len(values)):
                    values.append(0)

            concatenated_values = pd.concat([pd.Series(v) for v in values], axis=0).values
            distance = wasserstein_distance(control, concatenated_values)
            all_datapoints[datapoint] = distance

    return all_datapoints

def take_metrics(input_dir):
    global max_count 
    max_count = 0
    counts = {}
    for root, dirs, files in os.walk(input_dir):
        for series_directory in dirs:
            series_path = os.path.join(root, series_directory)
            for file in os.listdir(series_path):
                file_match = re.compile(r"(.+)_\d+.csv").match(file)
                if file_match is None:
                    raise ValueError(f"{file}: datapoint not found in file name; should be in form [datapoint]_[trial_number].csv and in series directories under the given input_dir (I) positional argument")

                datapoint = re.compile(r"(.+)_\d+.csv").match(file).group(1)
                if datapoint is None:
                    raise ValueError(f"{file}: datapoint not found in file name; should be in form [datapoint]_[trial_number].csv and in series directories under the given input_dir (I) positional argument")

                if datapoint not in counts:
                    counts[datapoint] = 0

                counts[datapoint] += 1
                if counts[datapoint] > max_count:
                    max_count = counts[datapoint]

    return max_count


def main():
    args = parse_args()

    print("checking input validity")
    if not os.path.exists(args.input_dir):
        print(f"input directory not found: {args.input_dir}")
        raise FileNotFoundError(args.input_dir)

    if len(os.listdir(args.input_dir)) == 0:
        print("input directory is empty")
        raise FileNotFoundError(args.input_dir)

    take_metrics(args.input_dir)
    if max_count <= 3:
        print(f"warning: only (at most) {max_count} parseable trial runs per datapoint could be found, this is likely a mistake -- check that the format of your file names is [datapoint]_[trial_number].csv")

    print("input valid")

    csvcols = ["day", "infectious_count"]
    control = pd.read_csv(args.control_csv, usecols=csvcols)["infectious_count"]

    distribution_totals = {}
    all_datapoints = {}

    for i in track(range(200), description="processing"):
        current_timestep = compute_earthmover_distance(args.input_dir, args.control_csv, i)
        all_datapoints[i] = current_timestep
        for key in current_timestep.keys():
            if not key in distribution_totals:
                distribution_totals[key] = current_timestep[key]
            else:
                distribution_totals[key] += current_timestep[key]

    #print(distribution_totals)
    print("complete")

    os.makedirs(args.output_dir, exist_ok=True)
    data = pd.DataFrame({k: [v] for k, v in distribution_totals.items()})
    data.to_csv(f"{args.output_dir}/totals.csv")

    data = pd.DataFrame(all_datapoints)
    data.to_csv(f"{args.output_dir}/data.csv")
            


if __name__ == "__main__":
    main()
