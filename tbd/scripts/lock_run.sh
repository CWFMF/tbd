#!/bin/bash
DIR=`dirname $(realpath "$0")`
export FORCE_RUN=
export IS_CRONJOB=${IS_CRONJOB}
/usr/bin/flock -n /appl/data/update.lock ${DIR}/update.sh $*