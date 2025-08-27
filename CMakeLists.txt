cmake_minimum_required(VERSION 3.50..4.1 FATAL_ERROR)

set(VERSION "0.0.1")
set(PROJECT_NAME "CarRace")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# VCPKG INTEGRATION
find_program(VCPKG_EXECUTABLE vcpkg)

if(NOT EXISTS ${VCPKG_EXECUTABLE})
  include(FetchContent)
  FetchContent_Declare(
    vcpkg
    GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
    GIT_TAG 2025.07.25
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(vcpkg)

  set(VCPKG_EXECUTABLE ${vcpkg_SOURCE_DIR}/vcpkg${CMAKE_HOST_EXECUTABLE_SUFFIX})
  if(NOT EXISTS ${VCPKG_EXECUTABLE})
    message(STATUS "Bootstrapping vcpkg: ${VCPKG_EXECUTABLE}")
    if(WIN32)
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E env ${vcpkg_SOURCE_DIR}/bootstrap-vcpkg.bat
        WORKING_DIRECTORY ${vcpkg_SOURCE_DIR})
    else()
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E env bash
                ${vcpkg_SOURCE_DIR}/bootstrap-vcpkg.sh
        WORKING_DIRECTORY ${vcpkg_SOURCE_DIR})
    endif()
    set(ENV{VCPKG_ROOT} "${vcpkg_SOURCE_DIR}/scripts/buildsystems")
    message(STATUS "VCPKG executable bootstrapped: ${VCPKG_EXECUTABLE}")
  else()
    message(STATUS "vcpkg executable already exists: ${VCPKG_EXECUTABLE}")
  endif()

  set(ENV{VCPKG_ROOT} "${vcpkg_SOURCE_DIR}")
  message(STATUS "VCPKG_ROOT set to: $ENV{VCPKG_ROOT}")
else()
  message(STATUS "Using system vcpkg executable: ${VCPKG_EXECUTABLE}")
endif()

set(CMAKE_TOOLCHAIN_FILE
    $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
    CACHE STRING "Vcpkg toolchain file")
message(STATUS "Using CMake toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
# END VCPKG INTEGRATION

project(
  ${PROJECT_NAME}
  VERSION ${VERSION}
  LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(FreeGLUT CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(
  ${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,
                          FreeGLUT::freeglut, FreeGLUT::freeglut_static>)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(CTest)
enable_testing()
