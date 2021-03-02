"""
Algorithms that perform partitioning on people set.
"""

# Now assign the people to clusters
def count_in_list(listv, cluster):
    """ Returns how many elements of listv are in cluster. """
    count = 0
    for v in listv:
        if v in cluster:
            count += 1
    return count

def greedy_affinity_to_clusters(clusters, limit_people, people_location_visit_graph):
    """ Greedily assigns people to clusters based on which non-full cluster
    that person has the greatest number of shared visits to. 

    Args:
        clusters: The location ids in each cluster as an array of sets.
        limit_people: The maximum people allowed in a group, as an int.
        people_location_visit_graph: 2D array (num_locations, num_people) that 
            tracks the number of times person X visited location Y.

    Returns:
        people_clusters: The people ids assigned to each cluster, as an array of
        sets.
    """
    # Create return data structure.
    people_clusters = [set() for _ in range(len(clusters))]

    # Greedily assign people based on their total number of visits.
    people_sorted_by_number_of_visits = []
    for index, count in enumerate(people_location_visit_graph.sum(axis=1)):
        people_sorted_by_number_of_visits.append((count, index))
    people_sorted_by_number_of_visits.sort(reverse=True)
        
    # For each person assign them to non-full cluster for which they have the
    # greatest number of visits to. O(PL). Switching this logic to sparse
    # matrices would lead to significant speedup.
    for index, num in enumerate(people_sorted_by_number_of_visits):
        _, person_index = num
        visit_locations = people_location_visit_graph[:, person_index].nonzero()[0]
        best_cluster = 0
        max_matches = -1
        for index, cluster in enumerate(clusters):
            if len(people_clusters[index]) < limit_people:
                matches = count_in_list(visit_locations, cluster)
                if matches > max_matches:
                    max_matches = matches
                    best_cluster = index
        people_clusters[best_cluster].add(person_index)
    return people_clusters