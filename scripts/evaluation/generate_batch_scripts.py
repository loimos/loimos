#!/usr/bin/env python3
# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""
Generates batch scripts for evaluation.
"""

import argparse
import pandas as pd
import math

# Configuration
PROCESSORS_PER_NODE = 16
UPPER_TIME_LIMIT = "06:00" # 6 hours
SIMULATION_DAYS = 16

BASELINE_SCRIPT = \
f"""
#!/bin/bash
#SBATCH -C haswell
#SBATCH -N {{NUM_NODES}}
#SBATCH --ntasks-per-node={PROCESSORS_PER_NODE}
#SBATCH -t {UPPER_TIME_LIMIT}
#SBATCH --output={{OUTPUT}}

cd /global/homes/i/ianjc/

module load openmpi/4.0.1 hpctoolkit rca
module unload craype-hugepages2M darshan
module load craype-hugepages8M

export CHARM_HOME=/global/homes/i/ianjc/libs/charm-v6.10.2
export PROTOBUF_HOME=/global/homes/i/ianjc/libs/install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PROTOBUF_HOME/lib

mpirun -np {{num_processes}} /global/homes/i/ianjc/loimos/src/loimos 1 {{people_grid_x}} {{people_grid_y}} {{location_grid_x}} {{location_grid_y}} {{degree}} {{people_chares}} {{location_chares}} {SIMULATION_DAYS} ../data/disease_models/covid19_onepath.textproto
"""

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Creates batch scripts given testing file.')
    
    # Positional/required arguments:
    parser.add_argument('tests_to_run',
        help='The path to a csv defined the tests to run.')
    parser.add_argument('output_folder',
        help='Where to output batch files.')
    args = parser.parse_args()
    assert(args.output_folder[-1] == "/")

    # Load the dataset.
    tests_to_run = pd.read_csv(args.tests_to_run)
    runs = []

    for _, row in tests_to_run.iterrows():
        num_procs = row['number_of_processes']
        num_nodes = math.ceil(num_procs / PROCESSORS_PER_NODE)

        # Calculate people grid dimensions
        output_file_name = f"loimos_run_{num_procs}_people_{row['people_grid_x']}_by_{row['people_grid_y']}_location_{row['location_grid_x']}_by_{row['location_grid_y']}_{row['distribution']}({row['degree']})"
        batch_file = args.output_folder + output_file_name + ".sh"
        with open(batch_file, "w") as f:
            f.write(BASELINE_SCRIPT.format_map({
                'num_processes': num_procs,
                'NUM_NODES': num_nodes,
                'people_grid_x': row['people_grid_x'],
                'people_grid_y': row['people_grid_y'],
                'location_grid_x': row['location_grid_x'],
                'location_grid_y': row['location_grid_y'],
                'degree': row['degree'],
                'people_chares': num_procs,
                'location_chares': num_procs,
                'OUTPUT': output_file_name
            }))

        # Print run to console.
        print(f"sbatch {batch_file}")
    
    

