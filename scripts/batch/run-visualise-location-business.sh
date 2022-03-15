#!/bin/bash
#SBATCH -q normal
#SBATCH -p standard
#SBATCH -t 30
#SBATCH -N 1
#SBATCH --account=biocomplexity

RAW_DIR=../../data/populations/wy_old
PARTITIONED_DIR=./

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

../partitioning/visualise_location_busyness.py  ${RAW_DIR} 2304
mv partition_weights.pdf before_lb_partition_weights.pdf

../partitioning/visualise_location_busyness.py  ${PARTITIONED_DIR} 2304
mv partition_weights.pdf after_lb_partition_weights.pdf
