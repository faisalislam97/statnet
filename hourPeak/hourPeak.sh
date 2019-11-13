#! /bin/bash

echo "Starting the job"

FILENAME="/home/faisal/hourPeak/hourPeak"

if [ -e "$FILENAME" ];then
	`sudo "$FILENAME"` # >> "/home/faisal/hourPeak/results"`
	echo "Job done"
else
	echo "$FILENAME not found"
	echo "Aborting job"
fi
