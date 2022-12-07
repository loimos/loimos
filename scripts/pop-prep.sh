#!/usr/bin/env bash


# While maybe fun to know the OSTYPE, MacOS has an inferior/
# incompatible readlink tool. To remedy this, one can run this on Mac
# after having installed greadlink:

case "$OSTYPE" in
  solaris*) os="SOLARIS" ;;
  darwin*)  os="DARWIN"; alias readlink=greadlink ;;
  linux*)   os="LINUX" ;;
  bsd*)     os="BSD" ;;
  msys*)    os="WINDOWS" ;;
  *)        os="unknown: $OSTYPE" ;;
esac

version="1_9"

# The assumption is that the collection of scripts reside in the same
# directory given by:
script_dir="$(dirname "$(readlink -f "$0")")"

echo "# ----------------------------------------------------------------------"
echo "# Starting invocation of $0"

if [ ! -f ${script} ]
then
    echo "# Error: sbatch script not found. Terminating."
    echo "# Script: <${script}>"
    exit 1
fi


help_function()
{
   echo "$(cat << EOF
Usage: $0 -s subregion -d dir
    -s subregion file prefix
    -d path to data directory
    -h Display help
EOF
)"
}

umask_okay="n"
network_tasks="512"

# Use the current directory as the default and clear other varibales
unset -v region
in_dir=$(pwd)
out_dir=$(pwd)
NO_RESIDENCE_OFFSET=false

while getopts "hns:i:o:?" opt
do
  case "$opt" in
    h ) help_function; exit ;;
    s ) region=$OPTARG ;;
    i ) in_dir=$OPTARG ;;
    o ) out_dir=$OPTARG ;;
    n ) NO_RESIDENCE_OFFSET=true ;;
    ? ) echo "Unrecognized commandline option <${opt}>" ; help_function ;;
  esac
done

echo "#> processing $region data in $in_dir, saving data to $out_dir"

person_filename=${in_dir}/base_population/${region}_person.csv
household_filename=${in_dir}/base_population/${region}_household.csv
residence_assignment_filename=${in_dir}/home_location_assignment/${region}_household_residence_assignment.csv
activity_locations_filename=${in_dir}/locations/${region}_activity_locations.csv
residence_locations_filename=${in_dir}/locations/${region}_residence_locations.csv
adult_location_assignment_filename=${in_dir}/location_assignment/weekly/${region}_adult_activity_location_assignment_week.csv
child_location_assignment_filename=${in_dir}/location_assignment/weekly/${region}_child_activity_location_assignment_week.csv

fixed_location_assignment_filename=${out_dir}/${region}_activity_location_assignment_week_final.csv
fixed_residence_locations_filename=${out_dir}/${region}_residence_locations_final.csv
fixed_activity_locations_filename=${out_dir}/${region}_activity_locations_final.csv
fixed_residence_assignment_filename=${out_dir}/${region}_household_residence_assignment_final.csv

HOME_SHIFT=1000000000
echo Using HOME_SHIFT of ${HOME_SHIFT}

# Combine activity files into one. Could check that line counts match.
# has format: hid,pid,activity_number,activity_type,start_time,duration,lid,longitude,latitude,travel_mode
# target format: daynum,pid,activity_number,activity_type,start_time,end_time,duration,lid

# Some visits are not home activities, but still occur at locations with an lid
# beyond that of the last activity location; we assume these are at home
# locations
max_activity_lid=$(($(wc -l < ${activity_locations_filename}) - 1))

awk -F, -v home_shift=${HOME_SHIFT} -v max_activity_lid=${max_activity_lid} \
'BEGIN{OFS=","} {
if( $4 == 1 || $7 > max_activity_lid ){ $7+=home_shift };
if( NR == 1 ){ print "daynum,pid,activity_number,activity_type,start_time,end_time,duration,lid" };
if( FNR > 1 ){ end_time=$5+$6; daynum=int($5/(24*60*60)); print daynum,$2,$3,$4,$5,end_time,$6,$7}
}' ${adult_location_assignment_filename} ${child_location_assignment_filename} > ${fixed_location_assignment_filename} # ${region}_gidi_visits.csv

# Fix the header of the activity location file:
awk -F, 'BEGIN{OFS=","}; { if(FNR==1){ $1="lid"}; {print}}' ${activity_locations_filename} > ${fixed_activity_locations_filename}

# Fix the household to residence assignment: update header name and add offset.
awk -F, -v home_shift=${HOME_SHIFT} 'BEGIN{ OFS=","} { if(NR>1){$2+=home_shift; print} else {$2="lid"; print } }' ${residence_assignment_filename} > ${fixed_residence_assignment_filename}

# Some states seem to have the shift already applied to home indices
if ${NO_RESIDENCE_OFFSET}; then
  HOME_SHIFT=0
fi
# Create residence locations file with lid offsets:
awk -F, -v home_shift=${HOME_SHIFT} 'BEGIN{ OFS=","} { if(NR>1){$1+=home_shift; print} else {$1="lid"; print } }' ${residence_locations_filename} > ${fixed_residence_locations_filename}
