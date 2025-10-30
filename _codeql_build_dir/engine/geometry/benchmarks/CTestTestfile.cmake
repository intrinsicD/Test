# CMake generated Testfile for 
# Source directory: /home/runner/work/Test/Test/engine/geometry/benchmarks
# Build directory: /home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[geometry_normals_benchmark]=] "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/engine_geometry_benchmarks" "--resolution" "256" "--iterations" "128" "--output" "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/geometry_normals_benchmark.json")
set_tests_properties([=[geometry_normals_benchmark]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\"benchmark\":\"geometry_normal_recompute\"" _BACKTRACE_TRIPLES "/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;11;add_test;/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;0;")
add_test([=[geometry_frustum_benchmark]=] "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/engine_geometry_frustum_benchmark" "--boxes" "200000" "--iterations" "256" "--output" "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/geometry_frustum_benchmark.json")
set_tests_properties([=[geometry_frustum_benchmark]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\"benchmark\":\"geometry_frustum_culling\"" _BACKTRACE_TRIPLES "/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;34;add_test;/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;0;")
add_test([=[geometry_shape_intersection_benchmark]=] "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/engine_geometry_shape_intersection_benchmark" "--pairs" "100000" "--iterations" "64" "--output" "/home/runner/work/Test/Test/_codeql_build_dir/engine/geometry/benchmarks/geometry_shape_intersection_benchmark.json")
set_tests_properties([=[geometry_shape_intersection_benchmark]=] PROPERTIES  PASS_REGULAR_EXPRESSION "\"benchmark\":\"geometry_shape_intersections\"" _BACKTRACE_TRIPLES "/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;57;add_test;/home/runner/work/Test/Test/engine/geometry/benchmarks/CMakeLists.txt;0;")
