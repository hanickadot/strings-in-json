cmake_minimum_required(VERSION 3.25)

project(json-normalize VERSION 1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
	catch2
	GIT_REPOSITORY https://github.com/catchorg/catch2.git
	GIT_TAG        devel
)
FetchContent_MakeAvailable(catch2)

add_executable(run-tests test.cpp)
target_compile_features(run-tests PUBLIC cxx_std_20)
target_link_libraries(run-tests Catch2::Catch2WithMain)

add_custom_target(tests COMMAND run-tests --skip-benchmarks --colour-mode ansi DEPENDS run-tests)
add_custom_target(benchmark COMMAND run-tests --benchmark-no-analysis DEPENDS run-tests)

add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_20)

add_custom_target(run COMMAND main DEPENDS main)
