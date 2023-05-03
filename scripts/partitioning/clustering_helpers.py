# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#


def split_large_clusters(clusters, max_in_cluster):
    """ Breaks clusters that are above the maximum size into (n-1) full clusters
    and 1 partially fully cluster. """
    # Detects if any clusters are oversized and if so creates a list of new
    # clusters to replace that one.
    new_clusters = []
    for name, values in clusters.items():
        total_in_cluster = len(values)
        if total_in_cluster > max_in_cluster:
            temp = []
            for new_cluster_num, i in enumerate(
                range(0, total_in_cluster, max_in_cluster)
            ):
                temp.append(
                    (f"{name}_{new_cluster_num}", values[i : i + max_in_cluster],)
                )
            new_clusters.append((name, temp))

    # Replace single large clusters with batch of smaller ones
    for old_cluster_name, new_batch in new_clusters:
        del clusters[old_cluster_name]
        for name, values in new_batch:
            clusters[name] = values
    return clusters


def recombine_clusters(clusters, max_in_cluster):
    """ Given a list of complete and incomplete clusters, this function combines
    those clusters into n-1 full clusters and at most 1 partial cluster. """
    if len(clusters) <= 1:
        return clusters

    # We will shift data from the clusters at the back to the front
    keys = list(clusters.keys())
    c_front = 0
    c_back = len(keys) - 1
    while c_front != c_back:
        # Calculate how many elements the front cluster needs to be complete.
        lacking_from_front = max(0, max_in_cluster - len(clusters[keys[c_front]]))
        if lacking_from_front == 0:
            c_front += 1
            continue

        # May or may not be enough elements in the current last cluster to give.
        back_to_give = len(clusters[keys[c_back]])
        if lacking_from_front >= back_to_give:
            # Can shift all the keys from the back.
            clusters[keys[c_front]].extend(clusters[keys[c_back]])
            del clusters[keys[c_back]]
            c_back -= 1
            if lacking_from_front == back_to_give:
                if c_front == c_back:
                    return clusters
                c_front += 1
        else:
            # Back has too many to give so pop these off.
            for _ in range(lacking_from_front):
                clusters[keys[c_front]].append(clusters[keys[c_back]].pop(0))
            c_front += 1
    return clusters


# Clustering helper.
def get_max_in_set(row, unassigned):
    cur_max = None
    cur_index = None
    for pid in range(row.shape[0]):
        val = row[pid]
        if (cur_max is None or val > cur_max) and pid in unassigned:
            cur_max = val
            cur_index = pid
    return cur_max, int(cur_index)
