#!/bin/sh

while [ 1 = 1 ]
do
  LAUNCH_CMD=`./Carousel`
  if [ $? = 0 ]
  then
    eval $LAUNCH_CMD
  else
    exit
  fi
done
