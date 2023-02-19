cmake_minimum_required(VERSION 3.25)

project(json-normalize VERSION 1.0 LANGUAGES CXX)

find_package(Catch2 REQUIRED)

add_executable(run-tests test.cpp)
target_compile_features(run-tests PUBLIC cxx_std_20)
target_link_libraries(run-tests Catch2::Catch2WithMain)

add_custom_target(tests COMMAND run-tests --skip-benchmarks --colour-mode ansi DEPENDS run-tests)
add_custom_target(benchmarks COMMAND run-tests --benchmark-no-analysis --colour-mode ansi DEPENDS run-tests)