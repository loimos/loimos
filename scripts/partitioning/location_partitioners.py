# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#

"""
Algorithms that perform clustering on the location set.
"""
import random
import math
from collections import defaultdict, deque
from sklearn.metrics.pairwise import cosine_similarity
from scipy import sparse

import clustering_helpers as ch


def geo_cluster_recursive(locations_to_group, c_depth, max_in_cluster, MAX_DEPTH=3):
    """ Recursive helper for geo partitioning scheme."""
    # Always will form initial groups based on initial depth.
    clusters = defaultdict(list)
    for lid, admin_levels in locations_to_group:
        # Move next location to its subcluster.
        clusters[admin_levels[c_depth]].append((lid, admin_levels))

    if c_depth == MAX_DEPTH:
        # If we are that bottom then just split any clusters that are too big
        clusters = ch.split_large_clusters(clusters, max_in_cluster)
        # And recombine.
        return ch.recombine_clusters(clusters, max_in_cluster)
    else:
        # At a higher depth, we do a subclustering for each group then
        # recombine as appropriate.
        new_clusters = {}
        for root_admin_level, locs_in_cluster in clusters.items():
            sub_clusters = geo_cluster_recursive(
                locs_in_cluster, c_depth + 1, max_in_cluster, MAX_DEPTH
            )

            for sub_cluster_name, sub_cluster in sub_clusters.items():
                new_clusters[f"{root_admin_level}_{sub_cluster_name}"] = sub_cluster
        return ch.recombine_clusters(new_clusters, max_in_cluster)


def geo_clustering(locations, admin_levels, total_partitions):
    # Turn pandas dataframe to list by admin levels.
    locations_to_group = []
    for _, loc in locations.iterrows():
        locations_to_group.append(
            (loc["lid"], (loc["admin1"], loc["admin2"], loc["admin3"], loc["admin4"]))
        )

    # Make recursive call to generate clusters.
    locations_per_partition = math.ceil(locations.shape[0] / total_partitions)
    clusters_with_admin = geo_cluster_recursive(
        locations_to_group, 1, locations_per_partition, admin_levels - 1
    )

    # Strip admin codes from clusters and return as a set.
    clusters = []
    for _, values in clusters_with_admin.items():
        clusters.append(set([lid for lid, _ in values]))
    return clusters


def graph_partioning_clustering(
    locations, total_partitions, people_location_visit_graph
):
    # Calculate location similarity matrix.
    location_similarity = cosine_similarity(
        sparse.csr_matrix(people_location_visit_graph)
    )

    # Misc setup
    clusters = [set() for _ in range(total_partitions)]
    locations_per_partition = math.ceil(locations.shape[0] / total_partitions)

    # Track
    unassigned_locations = set(range(locations.shape[0]))
    for curr_cluster in range(1, total_partitions + 1):
        # Choose a random starting part
        curr_point = int(random.sample(unassigned_locations, 1)[0])

        # We will randomly select a next best location to include given the
        # current cluster.
        points_in_cluster = deque()
        curr_in_set = 1
        max_in_set = (
            locations_per_partition
            if curr_cluster != total_partitions
            else (locations.shape[0] % locations_per_partition)
        )
        while curr_in_set < max_in_set:
            # Assign current point first.
            last_in_queue = len(points_in_cluster)
            points_in_cluster.append(curr_point)
            if curr_point not in unassigned_locations:
                unassigned_locations.remove(curr_point)
                clusters[curr_cluster - 1].add(curr_point)

            # Greedily pick the next largest path
            _, next_index = ch.get_max_in_set(
                location_similarity[curr_point], unassigned_locations
            )

            # Backtrack if no valid paths
            if next_index is None:
                # If cannot backtrack anymore than pick a new random point
                if last_in_queue <= 1:
                    next_index = random.sample(unassigned_locations, 1)[0]
                else:
                    points_in_cluster.pop()
                    next_index = points_in_cluster.pop()
            else:
                curr_in_set += 1

            # Assign next
            curr_point = next_index
        print(f"Built Cluster {curr_cluster} of {total_partitions}", end="\r")
    # Assign any remaining points to the final cluster.
    if len(unassigned_locations):
        clusters.append(unassigned_locations)
    return clusters
