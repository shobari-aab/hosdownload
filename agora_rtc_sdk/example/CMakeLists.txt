cmake_minimum_required(VERSION 2.4)

project(AgoraSDKDemo)

include(${CMAKE_SOURCE_DIR}/scripts/env.cmake)
include(${CMAKE_SOURCE_DIR}/scripts/os.cmake)
include(${CMAKE_SOURCE_DIR}/scripts/check.cmake)

if("${MEM_CHECK}" STREQUAL "true")
  set(sanitizer -fsanitize=address)
endif()

set(AGORA_MEDIA_SDK_DIR ${CMAKE_SOURCE_DIR}/../agora_sdk)
set(AGORA_MEDIA_SDK_HEADER_DIR ${CMAKE_SOURCE_DIR}/../agora_sdk)

# Set common flags
set(CMAKE_DEBUG_POSTFIX "")
set(CMAKE_RELEASE_POSTFIX "")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -Wno-unused-command-line-argument ${sanitizer}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if("${LICENSE_CHECK}" STREQUAL "y")
  message("license check is enabled")
  add_definitions(-DLICENSE_CHECK)
endif()

# include header path
include_directories(
  "${CMAKE_SOURCE_DIR}"
  "${AGORA_MEDIA_SDK_HEADER_DIR}/include")

# Link common libs
link_directories(${AGORA_MEDIA_SDK_DIR})
link_libraries(${LIB_SDK} ${LIB_THREAD})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out/)

add_subdirectory(recorder)
add_subdirectory(recorder_local)
