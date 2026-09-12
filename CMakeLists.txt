cmake_minimum_required(VERSION 3.15)
project(aumedal_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# z3 C++ API is bundled with the z3-solver pip package. Override with -DZ3_ROOT=... if needed.
if(NOT Z3_ROOT)
  set(Z3_ROOT "$ENV{HOME}/.local/lib/python3.9/site-packages/z3")
endif()
set(Z3_INCLUDE "${Z3_ROOT}/include")
set(Z3_LIB "${Z3_ROOT}/lib/libz3.so")
if(NOT EXISTS "${Z3_INCLUDE}/z3++.h")
  message(FATAL_ERROR "z3++.h not found under ${Z3_INCLUDE}; pass -DZ3_ROOT=<dir containing include/ and lib/>")
endif()

set(AUMEDAL_SRC
  src/flow.cpp src/config.cpp src/circuit.cpp src/placement.cpp src/netpoints.cpp
  src/prelayout.cpp src/graph.cpp src/variables.cpp src/smt.cpp src/smt_spacing.cpp
  src/smt_net.cpp src/prelayout_blockage.cpp src/solve.cpp src/gds.cpp)

add_executable(flow ${AUMEDAL_SRC})
target_include_directories(flow PRIVATE ${Z3_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(flow PRIVATE ${Z3_LIB})
set_target_properties(flow PROPERTIES
  BUILD_RPATH "${Z3_ROOT}/lib"
  INSTALL_RPATH "${Z3_ROOT}/lib")
