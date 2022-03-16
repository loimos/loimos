#!/bin/bash
#SBATCH -q normal
#SBATCH -p largemem
#SBATCH -t 120
#SBATCH -N 1
#SBATCH --account=biocomplexity

STATE=${1}
NUM_PARTITIONS=${2}
if [ -z ${NUM_PARTITIONS} ]; then
  NUM_PARTITIONS=576
fi

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8

echo ../partitioning/folding_partition.py  ../../data/populations/${STATE} ${NUM_PARTITIONS}
time ../partitioning/folding_partition.py  ../../data/populations/${STATE} ${NUM_PARTITIONS}

mv locations.csv ${STATE}_locations.csv
