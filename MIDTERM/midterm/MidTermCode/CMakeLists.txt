cmake_minimum_required(VERSION 3.2)
project(MidTerm)

file(GLOB sources "*.cpp")

add_library(MidTerm STATIC ${sources})

INSTALL(TARGETS
  MidTerm
  DESTINATION ${CMAKE_SOURCE_DIR}/../qplum_install/lib
)
