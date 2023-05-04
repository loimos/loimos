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
python3 generate.py --strategy=barabasi_albert --parameters=100,5,233
Generates a Barabasi Albert random graph with 100 nodes and mean degree 5, and uses the
amount of people (233) to generated a poisson distribution of people per location
centered around lambda = num_people/num_locations. This would transcribe to a
100 location graph, with a number of people roughly around the given num_people
parameter each with about 5 visits per day.

Example Usage 2
python3 generate.py --strategy=watts_strogatz --parameters=100,5,0.3

Note: By default, this saves the generated files to
../../data/populations/{strategy}_{parameters} and copies over the textproto
templates found in ../../data/textproto_templates/generated_data_templates.
In order to save to another directory, pass in --out_dir <path to dir>, or
pass in --out_dir "" to skip this step entirely and just save to the current
directory.
"""

import random_graphs
import translation_strategies
from absl import app
from absl import flags

FLAGS = flags.FLAGS
TRANSLATION_STRATEGY = flags.DEFINE_string(
    "strategy",
    "",
    "The translation strategy to use. One of barabasi_albert or watts_strogatz",
)
PARAMETERS = flags.DEFINE_string(
    "parameters", "", "The random graph parameters for the chosen strategy."
)
OUT_DIR = flags.DEFINE_string(
    "out_dir",
    "../../data/populations/{strategy}_{parameters}",
    "directory to save output files to [{strategy} and {parameters} will be "
    + " replaced with the values passed to those respective flags",
)
TEMPLATE_DIR = flags.DEFINE_string(
    "template_dir",
    "../../data/textproto_templates/generated_data_templates",
    "specifies where to find the textproto templates which should be copied"
    + " over to <out_dir>",
)


def main(unused_argv):
    del unused_argv

    parameters = PARAMETERS.value.split(",")

    # replace CLI arguements with their actual values to ensure a unique
    # name for the output directory
    out_dir = OUT_DIR.value.format(
        strategy=TRANSLATION_STRATEGY.value, parameters="_".join(parameters)
    )

    if TRANSLATION_STRATEGY.value == "barabasi_albert":
        if len(parameters) != 3:
            raise ValueError("Incorrect number of parameters provided.")
        num_nodes, mean_degree, num_people = parameters
        mean_people = int(num_people) / int(num_nodes)
        graph = random_graphs.generate_barabasi_albert(int(num_nodes), int(mean_degree))
        translation_strategies.graph_to_disease_model(
            graph, out_dir, TEMPLATE_DIR.value, int(num_nodes), mean_people
        )
    elif TRANSLATION_STRATEGY.value == "watts_strogatz":
        if len(parameters) != 4:
            raise ValueError("Incorrect number of parameters provided.")
        num_nodes, mean_degree_k, beta, num_people = parameters
        mean_people = int(num_people) / int(num_nodes)
        graph = random_graphs.generate_watts_strogatz(
            int(num_nodes), int(mean_degree_k), float(beta)
        )
        translation_strategies.graph_to_disease_model(
            graph, out_dir, TEMPLATE_DIR.value, int(num_nodes), mean_people
        )
        print(f"Synthetic population saved to {out_dir}")
    else:
        raise ValueError("Unknown translation strategy: %s", TRANSLATION_STRATEGY.value)


if __name__ == "__main__":
    app.run(main)
