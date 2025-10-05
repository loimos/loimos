#!/bin/bash
#SBATCH -N 1
#SBATCH --ntasks-per-node=8
#SBATCH -A bhatele-lab-cmsc
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH -t 00:30:00

LOIMOS_HOME=/home/ysakhark/loimos/loimos

mpirun --mca opal_warn_on_missing_libcuda 0 -n 8 primality 10
