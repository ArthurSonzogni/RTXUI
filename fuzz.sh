#!/bin/bash

dir=$(pwd)/fuzz_coverage
mkdir -p $dir

mkdir build_fuzz -p
cd build_fuzz

export CC=clang
export CXX=clang++
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DFUZZTEST_FUZZING_MODE=ON \
  -DRTXUI_BUILD_TESTS=ON \
  -DRTXUI_BUILD_EXAMPLES=OFF \
  -DRTXUI_BUILD_FUZZERS=ON \
  -DCMAKE_GENERATOR=Ninja

ninja

tests=(
  "XML.TestXML"
)

for test in "${tests[@]}"; do
  echo "Running test $test"
  coverage_dir=$dir/$test/coverage
  crashes_dir=$dir/$test/crashes
  corpus_dir=$dir/$test/corpus
  mkdir -p $coverage_dir
  mkdir -p $crashes_dir
  mkdir -p $corpus_dir

  export FUZZTEST_TESTSUITE_OUT_DIR=$corpus_dir
  ./rtxui_fuzzer --fuzz $test --fuzz_for=20s --stack_limit_kb=100000 --corpus_database_path=$corpus_dir
  #./build_fuzz/rtxui_fuzzer --fuzz $test --fuzz_for=10s --replay_coverage_inputs --stack_limit_kb=100000 --corpus_database_path=$corpus_dir
  #gdb --args ./cpp_react_tui_fuzzer --fuzz $test --fuzz_for=10s --replay_coverage_inputs --stack_limit_kb=100000
done
