import numpy as np
import pandas as pd
import argparse

import location_partitioners as lp
import people_partitioners as pp

import sys
import os
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils import id_remapper

LOCATION_SCHEMES = ["VISIT", "GEO"]
PEOPLE_SCHEMES = ["GREEDY"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Scripts to perform pre-run load balancing.")
    parser.add_argument("location_method", help=f"Location partitioning scheme to use: {LOCATION_SCHEMES}")
    parser.add_argument("people_method", help=f"People partitioning scheme to use: {PEOPLE_SCHEMES}")
    parser.add_argument("num_people_partitions")
    parser.add_argument("num_location_partitions")
    parser.add_argument("data_path", help=f"Path to root datafile.")
    parser.add_argument("output_path", help=f"Where to write rewritten files.")
    args = parser.parse_args()
    assert(args.location_method in LOCATION_SCHEMES)
    assert(args.people_method in PEOPLE_SCHEMES)
    
    # Read datafiles.
    people = pd.read_csv(f"{args.data_path}/people.csv")
    locations = pd.read_csv(f"{args.data_path}/locations.csv")
    visits = pd.read_csv(f"{args.data_path}/visits.csv")

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
    people.to_csv(f"{args.output_path}/people.csv", index=False)
    locations.to_csv(f"{args.output_path}/locations.csv", index=False)
    visits.to_csv(f"{args.output_path}/visits.csv", index=False)


