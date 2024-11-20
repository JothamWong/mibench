#!/bin/bash

PERF_COMMAND="perf stat -e branch-instructions,branch-misses,cache-misses,cache-references,cpu-cycles,instructions,stalled-cycles-frontend,alignment-faults,cgroup-switches,context-switches,cpu-clock,cpu-migrations,major-faults,minor-faults,page-faults,task-clock,duration_time,user_time,system_time,L1-dcache-loads,L1-dcache-load-misses,L1-dcache-prefetches,L1-icache-loads,L1-icache-load-misses,dTLB-loads,dTLB-load-misses,iTLB-loads,iTLB-load-misses,branch-loads,branch-loads-misses"

OUTPUT_DIR="../perf_reports"
mkdir -p $OUTPUT_DIR

export PLATFORM="linux"

# perf redirects to stderr by default
# We are using the input generation commands as specified in the Makefile
echo "Running basicmath"
export BASICMATH_RUNS=20
$PERF_COMMAND ./run_basicmath.sh 2> $OUTPUT_DIR/basicmath_perf

echo "Running bitcount"
export BITCOUNT_INPUT=140000000
$PERF_COMMAND ./run_bitcount.sh 2> $OUTPUT_DIR/bitcount_perf

echo "Running qsort small"
python ../input_generation/generate_qsort_small_input.py 9500000
$PERF_COMMAND ./run_qsort_small.sh 2> $OUTPUT_DIR/qsort_small_perf

echo "Running qsort large"
python ../input_generation/generate_qsort_large_input.py 10000000
$PERF_COMMAND ./run_qsort_large.sh 2> $OUTPUT_DIR/qsort_large_perf

echo "Running susan"
python ../input_generation/generate_susan_input.py 2500 2500
$PERF_COMMAND ./run_susan.sh 2> $OUTPUT_DIR/susan_perf

echo "Running jpeg"
python ../input_generation/generate_jpeg_input.py 6500 6500
$PERF_COMMAND ./run_jpeg.sh 2> $OUTPUT_DIR/jpeg_perf

# echo "Running lame"
# python ../input_generation/generate_lame_input.py 700
# $PERF_COMMAND ./run_lame.sh 2> lame_perf

echo "Running typeset"
python ../input_generation/generate_typeset_input.py 7000000
$PERF_COMMAND ./run_typeset.sh 2> $OUTPUT_DIR/typeset_perf

echo "Running dijkstra"
python ../input_generation/generate_dijkstra_input.py 2000
$PERF_COMMAND ./run_dijkstra.sh 2> $OUTPUT_DIR/dijkstra_perf

echo "Running patricia"
python ../input_generation/generate_patricia_input.py 2000 5500
$PERF_COMMAND ./run_patricia.sh 2> $OUTPUT_DIR/patricia_perf

echo "Running stringsearch"
export STRINGSEARCH_RUNS=5500
$PERF_COMMAND ./run_stringsearch.sh 2> $OUTPUT_DIR/stringsearch_perf

echo "Running blowfish"
python ../input_generation/generate_blowfish_input.py 120000000
$PERF_COMMAND ./run_blowfish.sh 2> $OUTPUT_DIR/blowfish_perf

echo "Running sha"
python ../input_generation/generate_sha_input.py 1100000000
$PERF_COMMAND ./run_sha.sh 2> $OUTPUT_DIR/sha_perf

echo "Running crc"
python ../input_generation/generate_crc_input.py 1200000000
$PERF_COMMAND ./run_crc.sh 2> $OUTPUT_DIR/crc_perf

echo "Running FFT"
export FFT_WAVES=2048
export FFT_LENGTH=32768
$PERF_COMMAND ./run_fft.sh 2> $OUTPUT_DIR/fft_perf

echo "Running adpcm"
python ../input_generation/generate_adpcm_input.py 300000000
$PERF_COMMAND ./run_adpcm.sh 2> $OUTPUT_DIR/adpcm_perf

echo "Running gsm"
python ../input_generation/generate_gsm_input.py 51000000
$PERF_COMMAND ./run_gsm.sh 2> $OUTPUT_DIR/gsm_perf
