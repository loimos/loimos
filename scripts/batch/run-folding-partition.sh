#!/bin/bash
#SBATCH -q normal
#SBATCH -p largemem
#SBATCH -t 20
#SBATCH -N 1
#SBATCH --ntasks-per-node 16
#SBATCH --exclusive
#SBATCH --account=biocomplexity

STATE=${1}
NUM_PARTITIONS=${2}
if [ -z ${NUM_PARTITIONS} ]; then
  NUM_PARTITIONS=576
fi
ARGS=${3}
if [ -z ${ARGS} ]; then
  ARGS=
  #ARGS="-p average_daily_total"
fi

RAW_DIR=../../data/populations/${STATE}
PARTITIONED_DIR=../../data/populations/${STATE}_partitioned

if [ ! -d ${PARTITIONED_DIR} ]; then
 mkdir ${PARTITIONED_DIR}
 echo "Made dir ${PARTITIONED_DIR}"
 echo
fi

# All of the files in the original dir are important, so link to any that
# are missing
for file in ${RAW_DIR}/*; do
 filename=$(basename ${file})
 if [[ ${filename} == *".textproto" && ! -e ${PARTITIONED_DIR}/${filename} ]]; then
   cp ${RAW_DIR}/${filename} ${PARTITIONED_DIR}/${filename}
   echo "  Copied ${RAW_DIR}/${filename} to ${PARTITIONED_DIR}/${filename}"
 fi
done

module load gcc/9.2.0 cuda/11.0.228 openmpi/3.1.6 \
  openmpi/3.1.6 python/3.8.8 # mvapich2/2.3.3 

echo ../partitioning/folding_partition.py  ${RAW_DIR} -o ${PARTITIONED_DIR} -n ${NUM_PARTITIONS} ${ARGS}
time ../partitioning/folding_partition.py  ${RAW_DIR} -o ${PARTITIONED_DIR} -n ${NUM_PARTITIONS} ${ARGS} -nt 16
