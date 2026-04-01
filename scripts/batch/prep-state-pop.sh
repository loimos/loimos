#!/bin/bash
#SBATCH -p standard
#SBATCH -q normal
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -t 10
#SBATCH --account=biocomplexity

export PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos

echo $1

state=md
in_dir=$PROJECT_ROOT/us-population-data/raw-data/${state}
out_dir=$PROJECT_ROOT/us-population-data/processed-data/${state}
time $PROJECT_ROOT/us-population-data/scripts/pop_prep.sh -s ${state} -i ${in_dir} -o ${out_dir}
