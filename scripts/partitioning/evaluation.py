# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#

import pandas as pd
import numpy as np


def evaluate_partitioning_scheme(
    path_to_dataframe,
    total_people,
    total_people_clusters,
    total_locations,
    total_location_clusters,
):
    # Load dataframe
    visits = pd.read_csv(path_to_dataframe)

    # Calculate counts per chare.
    people_per_chare = total_people // total_people_clusters
    locations_per_chare = total_locations // total_location_clusters

    # Calculate originating people partitions and destination location partitions.
    visits["lpartition"] = visits["lid"].apply(lambda x: x // locations_per_chare)
    visits["ppartition"] = visits["pid"].apply(lambda x: x // people_per_chare)
    visits["same_pe"] = visits["lpartition"] == visits["ppartition"]
    same_pe_visits = visits[visits["same_pe"]].shape[0] / visits.shape[0]

    # Calculate metric for equality of work.
    work = visits[["lpartition", "lid"]].groupby("lpartition").count()["lid"]
    equality_of_work = np.square(np.sum(work - np.mean(work))) / np.square(np.sum(work))
    return round(same_pe_visits, 5), round(equality_of_work, 5)
