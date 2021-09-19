# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

import snap
import random


def generate_barabasi_albert(num_nodes, edges_added_per_node):
    """Constructs a barabasi albert graph.

    Let N=num_nodes and M=edges_added_per_node

    We begin with a fully connected network of exactly M nodes.
    We add the remaining N-M nodes 1 at a time. For each new node,
    we randomly connect it to M existing nodes with a probability proportional
    to the degree of that node. (E.g. you are more likely to connect to nodes
    that already have a high degree).
    
    Args:
        num_nodes: Number of total nodes in the graph.
        edges_added_per_node: The number of edges to connect each 
    """
    graph = snap.TUNGraph.New()
    assert num_nodes >= 0
    assert edges_added_per_node >= 0

    # First M=edges_per_node nodes are fully connected.
    nodes = []
    for i in range(0, edges_added_per_node):
        graph.AddNode(i)
    # Generate initial fully connected network.
    for i in range(0, edges_added_per_node):
        for j in range(i, edges_added_per_node):
            nodes.append(i)
            nodes.append(j)
            graph.AddEdge(i, j)

    # For every additional node, we randomly connect it to exactly M
    for i in range(edges_added_per_node, num_nodes):
        nodes_to_connect_to = random.sample(population=nodes,
                                            k=edges_added_per_node)
        nodes.append(i)
        graph.AddNode(i)
        for other_node in nodes_to_connect_to:
            # We randomly select edges proportional to weight so add here
            nodes.append(i)
            nodes.append(other_node)
            graph.AddEdge(i, other_node)

    # Return graph object.
    return graph


def _get_lattice_neighbor(num_nodes, current_node, neighbor):
    """Helper to get the a neighor in a lattice ring."""
    proposed_neighbor = current_node + neighbor
    if proposed_neighbor >= num_nodes:
        return proposed_neighbor - num_nodes
    elif proposed_neighbor < 0:
        return proposed_neighbor + num_nodes
    else:
        return proposed_neighbor


def generate_watts_strogatz(num_nodes, mean_degree_k, beta):
    """Creates a watts strogatz random graph.
    
    The algorithm has two stages.
    In the first stage, we create a regular ring lattice.
    E.x https://www.researchgate.net/figure/FIGURE-A1-A-regular-ring-lattice-with-N-20-vertices_fig2_276832512
    A ring lattice is a structure where we lay out all N nodes in a circle and
    each node is connected to its closest K neighbors on the ring.

    In the second stage, we take every node and for of its K/2 rightmost edges,
    we rewire that edge (connect it to another node uniformly at random) with
    probability beta.
    """
    # Check parameters.
    assert 0 <= beta and beta <= 1
    assert num_nodes >= 0
    assert mean_degree_k >= 0
    # Add all the nodes the graph.
    graph = snap.TUNGraph.New()
    for i in range(0, num_nodes):
        graph.AddNode(i)

    # Stage 1: For each node, connect it to its inital neighbors.
    # Stage 2: Randomly rewire connections in the graph.
    # We combine these into a single loop for computation efficiency.
    for i in range(0, num_nodes):
        # Note only add forward neighbors as back neighbors will be added
        # eventually in the loop.
        for j in range(1, (mean_degree_k // 2) + 1):
            # Standard connection.
            if random.random() >= beta:
                # Standard connection.
                graph.AddEdge(i, _get_lattice_neighbor(num_nodes, i, j))
            else:
                # Choose a random edge.
                graph.AddEdge(i, random.randint(0, num_nodes - 1))
    return graph
