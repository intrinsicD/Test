# CMake generated Testfile for 
# Source directory: /home/runner/work/Test/Test/engine/physics/benchmarks
# Build directory: /home/runner/work/Test/Test/_codeql_build_dir/engine/physics/benchmarks
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[physics_collision_benchmark]=] "/home/runner/work/Test/Test/_codeql_build_dir/engine/physics/benchmarks/engine_physics_benchmarks" "--bodies" "128" "--steps" "512" "--dt" "0.0166667" "--seed" "1337" "--output" "/home/runner/work/Test/Test/_codeql_build_dir/engine/physics/benchmarks/physics_collision_benchmark.json")
set_tests_properties([=[physics_collision_benchmark]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\"benchmark\":\"physics_collision_throughput\"" _BACKTRACE_TRIPLES "/home/runner/work/Test/Test/engine/physics/benchmarks/CMakeLists.txt;11;add_test;/home/runner/work/Test/Test/engine/physics/benchmarks/CMakeLists.txt;0;")
