#! /bin/sh

echo "Starting the job"

FILENAME="/home/faisal/dayPeak/dayPeak"

if [ -e "$FILENAME" ];then
	`sudo "$FILENAME" >> "/home/faisal/dayPeak/results"`
	echo "Job done"
else
	echo "$FILENAME not found"
	echo "Aborting job"
fi
