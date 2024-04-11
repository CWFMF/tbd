#!/bin/bash
set -e

DIR="../data/test_output"
DIR_SIMS="${DIR}/sims"
DIR_LOGS="${DIR}/logs"
DURATION=0.1
MAX_DIR=359

# MAX_SPEED=50
# MAX_SLOPE=200
# STEP_SPEED=5
# STEP_DIR=5
# STEP_SLOPE=10

MAX_SPEED=50
MAX_SLOPE=60
STEP_SPEED=5
STEP_DIR=15
STEP_SLOPE=30

SPEEDS=(`seq -s ' ' 0 ${STEP_SPEED} ${MAX_SPEED}`)
DIRECTIONS=(`seq -s ' ' 0 ${STEP_DIR} ${MAX_DIR}`)
SLOPES=(`seq -s ' ' 0 ${STEP_SLOPE} ${MAX_SLOPE}`)
SPREAD_STEPS=( 20 15 10 5 1 0 )
HULLS=( quick normal off )

# SPEEDS=( 10 )
# SPEEDS=( 40 )
DIRECTIONS=( 0 )
SLOPES=( 0 )
SPREAD_STEPS=( 0 )
# HULLS=( quick normal )
HULLS=( quick  )

# # STEP_POWER_STEPS=1
# STEP_POWER_STEPS=2
# MAX_STEP_POWER=10
# STEP_POWERS=(`seq -s ' ' ${STEP_POWER_STEPS} ${STEP_POWER_STEPS} ${MAX_STEP_POWER}`)


# # STEP_MULT_POWER_STEPS=0.05
# STEP_MULT_POWER_STEPS=0.25
# MAX_STEP_MULT_POWER=0.99
# STEP_MULT_POWERS=(`seq -s ' ' ${STEP_MULT_POWER_STEPS} ${STEP_MULT_POWER_STEPS} ${MAX_STEP_MULT_POWER}`)


mkdir -p ${DIR_LOGS}
mkdir -p ${DIR_SIMS}
DIR_BUILD=/appl/tbd/build
VARIANT="$*"
if [ -z "${VARIANT}" ]; then
  VARIANT=Release
fi
echo Set VARIANT=${VARIANT}
/usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=${VARIANT} -S/appl/tbd -B${DIR_BUILD} -G "Unix Makefiles" \

function run_with_settings() {
    # echo "${*}"
    algo="nohull"
    spread_step="${1}"
    step_pad=`printf "%03g" ${spread_step}`
    if [ "${2}" == "off" ]; then
        do_hull="\/\/ "
    else
        do_hull=""
        if [ "${2}" == "quick" ]; then
            algo="quickhull"
            quick_hull=""
        else
            algo="condense16"
            quick_hull="\/\/ "
        fi
    fi
    # for step in "${STEP_POWERS[@]}" ; do
    #     for step_mult in "${STEP_MULT_POWERS[@]}" ; do
            # test="${algo}_${step_pad}"
            # echo "Running test for ${test}"
            sed -i "s/.*#define DO_HULL/${do_hull}#define DO_HULL/" src/cpp/ConvexHull.cpp
            sed -i "s/.*#define QUICK_HULL/${quick_hull}#define QUICK_HULL/" src/cpp/ConvexHull.cpp
            if [ "${spread_step}" == 0 ]; then
                sed -i "s/^.*#define STEP[^_\\n\\r]*$/\/\/ #define STEP/" src/cpp/FireSpread.cpp
            else
                sed -i "s/^.*#define STEP[^_\\n\\r]*$/#define STEP ${spread_step}/" src/cpp/FireSpread.cpp
            fi
            # sed -i "s/.*#define STEP_POWER.*/#define STEP_POWER ${step}/" src/cpp/FireSpread.cpp
            # sed -i "s/.*#define STEP_MULT_POWER.*/#define STEP_MULT_POWER ${step_mult}/" src/cpp/FireSpread.cpp
            /usr/bin/cmake --build ${DIR_BUILD} --config ${VARIANT} --target all -j 50 --
            for ws in "${SPEEDS[@]}" ; do
                for wd in "${DIRECTIONS[@]}" ; do
                    for slope in "${SLOPES[@]}" ; do
                        for aspect in "${DIRECTIONS[@]}" ; do
                            # C2_S000_A000_WD000_WS000
                            # f_step=`printf '%03g' ${step}`
                            # f_mult=`printf '%03g' ${step_mult}`
                            f_slope=`printf '%03d' ${slope}`
                            f_aspect=`printf '%03d' ${aspect}`
                            f_wd=`printf '%03d' ${wd}`
                            f_ws=`printf '%03d' ${ws}`
                            test="${algo}_${step_pad}_C2_S${f_slope}_A${f_aspect}_WD${f_wd}_WS${f_ws}"
                            # test="S${f_step}_P${f_mult}_${test}"
                            dir_path="${DIR_SIMS}/${test}"
                            if [ -d ${dir_path} ]; then
                                echo "Already ran test for ${dir_path}"
                            else
                                mkdir -p ${dir_path}
                                echo "Running test for ${test}"
                                ./tbd test ${dir_path} ${DURATION} ${slope} ${aspect} ${ws} ${wd} > ${DIR_LOGS}/${test}.log
                            fi
                        done
                    done
                done
            done
    #     done
    # done
}

# rm -rf ${DIR}
for hull in "${HULLS[@]}" ; do
    # echo "${hull}"
    for spread_step in "${SPREAD_STEPS[@]}" ; do
        # echo "${spread_step}"
        run_with_settings "${spread_step}" "${hull}"
    done
done

