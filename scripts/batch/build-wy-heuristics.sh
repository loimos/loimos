#!/bin/bash
#SBATCH -p parallel
#SBATCH -q normal
#SBATCH -N 2
#SBATCH --ntasks-per-node=40
#SBATCH -t 10
#SBATCH --account=biocomplexity

export PROJECT_ROOT=/home/arr2vg/biocomplexity/loimos
module load python

$PROJECT_ROOT/loimos/scripts/location_heuristics.py $PROJECT_ROOT/loimos/data/populations/wy/ -n 80
