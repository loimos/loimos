# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""Main class to generate synthetic population models.

Generates a random graph and translates it to a bi-partite visit schedule.

Random graphs supported: barabasi_albert or watts_strogatz
barabasi_albert is parameterized by (num_nodes, edges_added_per_node)
watts_strogatz is parameterized by (num_nodes, mean_degree_k, beta)
See random_graphs for information about these parameters.

Example Usage 1
python3 generate.py --strategy=barabasi_albert --parameters=100,5
Generates a Barabasi Albert random graph with 100 nodes and mean degree 5
and then transcribes this to a 100 location population graph with approximately
400 people each with about 5 visits per day.

Example Usage 2
python3 generate.py --strategy=watts_strogatz --parameters=100,5,0.3

"""

import random_graphs
import translation_strategies
from absl import app
from absl import flags

FLAGS = flags.FLAGS
TRANSLATION_STRATEGY = flags.DEFINE_string(
    "strategy", "", 
    "The translation strategy to use. One of barabasi_albert or watts_strogatz")
PARAMETERS = flags.DEFINE_string(
    "parameters", "",
    "The random graph parameters for the chosen strategy.")

def main(unused_argv):
    del unused_argv
    parameters = PARAMETERS.value.split(",")
    if TRANSLATION_STRATEGY.value == "barabasi_albert":
        if len(parameters) != 2:
            raise ValueError("Incorrect number of parameters provided.")
        num_nodes, mean_degree = parameters
        graph = random_graphs.generate_barabasi_albert(
            int(num_nodes), int(mean_degree))
        translation_strategies.graph_to_disease_model(graph)
    elif TRANSLATION_STRATEGY.value == "watts_strogatz":
        if len(parameters) != 3:
                raise ValueError("Incorrect number of parameters provided.")
        num_nodes, mean_degree_k, beta  = parameters
        graph = random_graphs.generate_watts_strogatz(
            int(num_nodes), int(mean_degree_k), float(beta))
        translation_strategies.graph_to_disease_model(graph)
    else:
        raise ValueError("Unknown translation strategy: %s",
                         TRANSLATION_STRATEGY.value)

if __name__ == '__main__':
      app.run(main)
