#!/bin/bash

##############
# ./scripts/mk_clean.sh Release

set -e

DIR_BUILD=/appl/tbd/build
# VARIANT="$*"
# if [ -z "${VARIANT}" ]; then
  VARIANT=Release
#   VARIANT=Debug
# fi
echo Set VARIANT=${VARIANT}
rm -rf ${DIR_BUILD} \
  && /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/tbd -B${DIR_BUILD} -G "Unix Makefiles" \
  && /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --

#################

dir="/appl/data/tmp/11N_54669"
# intensity=""
intensity="-i"
# intensity="--no-intensity"
dates="--output_date_offsets [1,2,3]"
# dates="--output_date_offsets {1,2,3}"
opts="--sim-area"
pushd ${dir}
set +e

rm probability*
rm intensity*
rm 0*.tif
rm *out*
rm sizes*
rm dem*
rm fuel*
rm simulation_area*
set -e

/appl/tbd/tbd . 2023-08-03 60.38716700000001 -116.272017 01:00 ${intensity} ${opts} --ffmc 86.0 --dmc 118.4 --dc 826.1 --apcp_prev 0 -v ${dates} --wx firestarr_11N_54669_wx.csv --perim 11N_54669.tif $*
popd
diff -rq ${dir}.orig ${dir}
