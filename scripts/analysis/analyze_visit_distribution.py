#!/usr/bin/env python
# coding: utf-8

# In[1]:

import os
import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import argparse

# Use a non-interactive backend so we don't have to worry about getting missing
# displays on remote systems (or setting up x-forwarding)
plt.switch_backend("Agg")


def parse_args():
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
        help="The directory in which the output files should be saved",
    )

    return parser.parse_args()


def analyze_by_location(locations, people, visits, output_dir):
    # cols = [f'total_on_day_{d}' for d in range(7)]
    # missing_mask = np.any(np.isnan(locations[cols]),axis=1)
    # locations.loc[missing_mask,cols] = 0

    # visit_counts = np.array(locations[cols]).flatten()
    visit_counts = np.array(locations["total_visits"])
    print(np.mean(visit_counts), np.std(visit_counts))
    print(np.quantile(visit_counts, [0.25, 0.5, 0.75]))

    visited_locations_mask = visit_counts != 0
    visited_location_counts = visit_counts[visited_locations_mask]
    print(visited_location_counts.shape, visit_counts.shape)

    fig, ax = plt.subplots(figsize=(10, 6))
    sns.histplot(visited_location_counts, bins=50, kde=False,
            log_scale=(False, True))
    plt.ylim(1, visited_location_counts.max())
    plt.xlabel("total_visits")
    plt.title(f"Total Visits Histogram")
    plt.savefig(os.path.join(output_dir, "visits_location_hist.pdf"))
    # sns.kdeplot(visited_location_counts, log_scale=(True,True))

    max_sim_visit_counts = np.array(locations["max_simultaneous_visits"])
    print(np.mean(max_sim_visit_counts), np.std(max_sim_visit_counts))
    print(np.quantile(max_sim_visit_counts, [0.25, 0.5, 0.75]))

    visited_locations_mask = max_sim_visit_counts != 0
    visited_location_counts = max_sim_visit_counts[visited_locations_mask]
    print(visited_location_counts.shape, visit_counts.shape)

    fig, ax = plt.subplots(figsize=(10, 6))
    sns.histplot(visited_location_counts, bins=50, kde=False,
            log_scale=(False, True))
    plt.ylim(1, visited_location_counts.max())
    plt.xlabel("max_simultaneous_visits")
    plt.title(f"Max Simultaneous Visit Histogram")
    plt.savefig(os.path.join(output_dir, "max_sim_visits_location_hist.pdf"))

    visits_by_location = visits.groupby(by="lid")
    visit_counts_by_location = visits_by_location[["lid", "start_time"]].count()

    #visit_counts_by_location["num_visit_sources"] = [
    #    len(np.unique(people.iloc[grouped["pid"]]["lid"]))
    #    for lid, grouped in visits_by_location
    #]
    #print(visit_counts_by_location)

    #fig, ax = plt.subplots(figsize=(10, 6))
    #sns.histplot(
    #    visit_counts_by_location,
    #    bins=50,
    #    x="num_visit_sources",
    #    kde=False,
    #    log_scale=(True, True),
    #)
    #plt.savefig(os.path.join(output_dir, "visit_sources_hist.pdf"))

    #print(
    #    visit_counts_by_location["num_visit_sources"].mean(),
    #    visit_counts_by_location["num_visit_sources"].std(),
    #)


def analyze_by_person(locations, people, visits, output_dir):
    visits["day"] = visits["start_time"] // 86400
    visit_counts = visits.groupby(by=["pid", "day"]).count().iloc[:, 0]
    print(visit_counts)

    print(np.mean(visit_counts), np.std(visit_counts))
    print(np.quantile(visit_counts, [0.25, 0.5, 0.75]))

    visited_people_mask = visit_counts != 0
    visited_person_counts = visit_counts[visited_people_mask]
    print(visited_person_counts.shape, visit_counts.shape)

    fig, ax = plt.subplots(figsize=(10, 6))
    sns.histplot(visited_person_counts, bins=50, kde=False, log_scale=(False, True))
    plt.savefig(os.path.join(output_dir, "visit_people_hist.pdf"))
    # sns.kdeplot(visited_person_counts, log_scale=(True,True))

    visits_by_person = visits.groupby(by="pid")
    visit_counts_by_person = visits_by_person.count().reset_index()
    visit_counts_by_person = visit_counts_by_person.rename(
        columns={"index": "pid", "hid": "count"}
    )

    print(visit_counts_by_person)

    fig, ax = plt.subplots(figsize=(10, 6))
    sns.histplot(visit_counts_by_person, bins=50, x="pid", y="count", kde=False)
    plt.savefig(os.path.join(output_dir, "visit_sources_hist.pdf"))

    print(
        visit_counts_by_person["start_time"].mean(),
        visit_counts_by_person["start_time"].std(),
    )


def main():
    args = parse_args()
    # input_dir = '../../data/populations/coc/'
    input_dir = args.input_dir
    output_dir = args.output_dir
    locations = pd.read_csv(os.path.join(input_dir, "locations.csv"))
    people = pd.read_csv(os.path.join(input_dir, "people.csv"))
    visits = pd.read_csv(os.path.join(input_dir, "visits.csv"))

    analyze_by_location(locations, people, visits, output_dir)
    # analyze_by_person(locations, people, visits, output_dir)


if __name__ == "__main__":
    main()
