#!/bin/bash
#SBATCH -p standard
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -t 10
#SBATCH --account=biocomplexity

export PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos
module load python

state=md
in_dir=$PROJECT_ROOT/us-population-data/raw-data/${state}
out_dir=$PROJECT_ROOT/us-population-data/processed-data/${state}
$PROJECT_ROOT/us-population-data/scripts/combine_household_data.py -s ${state}
