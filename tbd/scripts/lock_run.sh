#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=
/usr/bin/flock -n /appl/data/update.lock ${DIR}/update.sh $*