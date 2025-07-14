cmake_minimum_required(VERSION 3.10)
project(ZeroFlow)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include_directories(include)

add_executable(tests
    tests/test_fsm.cpp
)

add_executable(size_demo
    tests/size_demo.cpp
)

add_test(NAME ZeroFlowTests COMMAND tests)
