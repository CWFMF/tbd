#!/bin/bash
set -x

d="collected"
g="11N_47610"
r="m3_202308021910"

scripts/mk_clean.sh

rm -rf /appl/data/${d}

dir_to="/appl/data/${d}/${g}/${r}"
pushd /appl/data/sims/${r}/sims/${g}
mkdir -p ${dir_to}
cp * ${dir_to}/
popd

./get_grids.sh 00_pc_or_pdf_is_nonfuel
./get_grids.sh 00_pc_or_pdf_is_d1
./get_grids.sh 00_pc_or_pdf_is_d2
./get_grids.sh normal

pushd /appl/data/${d}
FILE_7Z="${g}.7z"
rm -rf ${FILE_7Z}
7za a ${FILE_7Z} ${g}/*
popd
