#!/usr/bin/env python3

import argparse
import os
import sys
import glob
import shutil

import pandas as pd

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils import ids  # noqa

def parse_args():
    parser = argparse.ArgumentParser(
        description="Samples a given set of locations (along with their "
        + "associated visits and people) from the given population"
    )

    # Positional/required arguments:
    parser.add_argument(
        "in_dir",
        metavar="I",
        help="A path to a directory containing data on the population to sample from",
    )
    parser.add_argument(
        "out_dir",
        metavar="O",
        help="The name of the directory where the sampled popualtion should be saved",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-s",
        "--to-sample",
        nargs="+",
        type=int,
        default=None,
        help="A list of location ids to sample"
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
        "-lp",
        "--locations-file",
        nargs="+",
        default="locations.csv",
        help="The name of the file containing location data within the "
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

def sample_population(people, locations, visits, lids_to_sample):
    locations = locations.iloc[lids_to_sample]
    visits = visits[visits["lid"].isin(lids_to_sample)]
    pids_to_sample = visits["pid"].unique()
    people = people.iloc[pids_to_sample]
    return people, locations, visits

def main():
    args = parse_args()

    in_dir = args.in_dir
    out_dir = args.out_dir

    people_in_file = os.path.join(in_dir, args.people_file)
    locations_in_file = os.path.join(in_dir, args.locations_file)
    visits_in_file = os.path.join(in_dir, args.visits_file)

    people = pd.read_csv(people_in_file)
    locations = pd.read_csv(locations_in_file)
    visits = pd.read_csv(visits_in_file)

    lids_to_sample = args.to_sample
    if lids_to_sample is None:
        busiest_lid = locations["max_simultaneous_visits"].argmax()
        #print(locations.iloc[busiest_lid])
        lids_to_sample = [int(busiest_lid)]
        #print(lids_to_sample)

    people, locations, visits = \
        sample_population(people, locations, visits, lids_to_sample)
    # Remap uses the indices to set pids and lids, so we need to make
    # sure they're contiguous
    people.reset_index(drop=True, inplace=True)
    locations.reset_index(drop=True, inplace=True)
    people, locations, visits = ids.remap(people, locations, visits)

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
        print(f"Created dir {out_dir}")
    for f in glob.glob(os.path.join(in_dir, "*.textproto")):
        shutil.copy(f, out_dir)
        print(f"  Copied {f} to {out_dir}")

    people_out_file = os.path.join(out_dir, args.people_file)
    locations_out_file = os.path.join(out_dir, args.locations_file)
    visits_out_file = os.path.join(out_dir, args.visits_file)

    people.to_csv(people_out_file, index=False)
    locations.to_csv(locations_out_file, index=False)
    visits.to_csv(visits_out_file, index=False)

    print(f"Sampled sub-population saved under {out_dir}")

    # Add an example run line in a batch script to the output dir
    # so that we can start running this right away
    with open(os.path.join(out_dir, "run.sh"), "w") as run_file: 
        num_people = people.shape[0]
        num_locations = locations.shape[0]
        real_out_path = os.path.realpath(out_dir)
        run_line = f"./charmrun +p4 ./loimos 0 {num_people} {num_locations} " \
            + f"1 1 7 7 sampled.out ../data/disease_models/covid19.textproto " \
            + f"{real_out_path}\n"
        run_file.write(run_line)
        print("Try running Loimos on this dataset from loimos/src with:")
        print(f"  {run_line}")
        

if __name__ == '__main__':
    main()
