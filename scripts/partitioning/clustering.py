# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#

import numpy as np
import pandas as pd
import argparse

import location_partitioners as lp
import people_partitioners as pp

import sys
import os

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils import id_remapper

LOCATION_SCHEMES = ["VISIT", "GEO"]
PEOPLE_SCHEMES = ["GREEDY"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Scripts to perform pre-run load balancing.")
    parser.add_argument("location_method", choices=LOCATION_SCHEMES,
        help="Location partitioning scheme to use")
    parser.add_argument("people_method", choices=PEOPLE_SCHEMES,
        help="People partitioning scheme to us")
    parser.add_argument("num_people_partitions")
    parser.add_argument("num_location_partitions")
    parser.add_argument("data_path", help="Path to root datafile.")
    parser.add_argument("output_path", help="Where to write rewritten files.")
    args = parser.parse_args()
    
    # Read datafiles.
    people = pd.read_csv(os.path.join(args.data_path, "people.csv"))
    locations = pd.read_csv(os.path.join(args.data_path, "locations.csv"))
    visits = pd.read_csv(os.path.join(args.data_path, "visits.csv"))

    # Build PeopleXLocation visit graph. Rows represent locations, columns are people
    # and the values are the number of times that person visits the location.
    people_location_visit_graph = np.zeros((locations.shape[0], people.shape[0]))
    for _, row in visits[['pid','lid','start_time']].groupby(['lid','pid']).count().reset_index().iterrows():
        people_location_visit_graph[row['lid']][row['pid']] = row['start_time']

    # Create location clusters.
    location_clusters = None
    if args.location_method == "VISIT":
        location_clusters = lp.graph_partioning_clustering(locations, int(args.num_location_partitions), people_location_visit_graph)
    else:
        location_clusters = lp.geo_clustering(locations, 4,  int(args.num_location_partitions))

    # Create people clusters.
    people_clusters = pp.greedy_affinity_to_clusters(location_clusters, int(args.num_people_partitions), people_location_visit_graph)

    # Remap all ids to new scheme.  
    people_remapped_ids = []
    for p_cluster in people_clusters:
        people_remapped_ids.extend(p_cluster)
    location_remapped_ids = []
    for l_cluster in location_clusters:
        location_remapped_ids.extend(l_cluster)
    people, locations, visits = id_remapper.remap(people.reindex(people_remapped_ids), locations.reindex(people_remapped_ids), visits, people_remapped_ids, location_remapped_ids)    

    # Output
    people.to_csv(os.path.join(args.output_path, "people.csv"), index=False)
    locations.to_csv(os.path.join(args.output_path, "locations.csv"), index=False)
    visits.to_csv(os.path.join(args.output_path, "visits.csv"), index=False)


