cmake_minimum_required(VERSION 3.16)
project(ValueBot)
set(CMAKE_CXX_STANDARD 17)

# Point at your OpenBW BWAPI checkout, e.g.
#   cmake -S . -B build -DBWAPI_DIR=/home/ubuntu/project/sc1/openbw-bwapi
set(BWAPI_DIR "/home/ubuntu/project/sc1/openbw-bwapi" CACHE PATH "OpenBW bwapi root")

add_library(ValueBot SHARED ValueBot.cpp)
target_include_directories(ValueBot PRIVATE ${BWAPI_DIR}/bwapi/include)
target_link_directories(ValueBot PRIVATE ${BWAPI_DIR}/build/lib)
target_link_libraries(ValueBot BWAPILIB)
set_target_properties(ValueBot PROPERTIES PREFIX "" SUFFIX ".so")
