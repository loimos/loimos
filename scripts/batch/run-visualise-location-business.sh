#!/bin/bash
#SBATCH -q normal
#SBATCH -p standard
#SBATCH -t 30
#SBATCH -N 1
#SBATCH --account=biocomplexity

<<<<<<< Updated upstream
STATE=${1}
NUM_PARTITIONS=${2}
if [ -z ${NUM_PARTITIONS} ]; then
  NUM_PARTITIONS=576
fi
ARGS=${3}
if [ -z ${ARGS} ]; then
  ARGS="-p average_daily_total"
fi

RAW_DIR=../../data/populations/${STATE}
PARTITIONED_DIR=../../data/populations/${STATE}_partitioned

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 mvapich2/2.3.3 \
  openmpi/3.1.6 python/3.8.8
=======
module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 \
  openmpi/3.1.6 python/3.8.8 #mvapich2/2.3.3
>>>>>>> Stashed changes

../partitioning/visualise_location_busyness.py  ${RAW_DIR} \
  -n ${NUM_PARTITIONS} -o ${RAW_DIR} ${ARGS}

../partitioning/visualise_location_busyness.py  ${PARTITIONED_DIR} \
  -n ${NUM_PARTITIONS} -o ${PARTITIONED_DIR} ${ARGS}
