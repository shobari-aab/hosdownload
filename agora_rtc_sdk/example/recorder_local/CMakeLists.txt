cmake_minimum_required(VERSION 3.10) # Modern CMake is better for path handling
project(AgoraRecorder)

# Define the base SDK path relative to this CMakeLists.txt
# This moves UP two levels from example/recorder_local/ to the root agora_rtc_sdk/
set(AGORA_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../agora_sdk")

# 1. Tell the compiler where to find headers
include_directories(
  "${PROJECT_SOURCE_DIR}"
  "/usr/include"
  "${AGORA_SDK_ROOT}/include"
)

# 2. Collect the source files
file(GLOB SAMPLE_RECORDER_FILES
     "${PROJECT_SOURCE_DIR}/main.cpp"
     "${PROJECT_SOURCE_DIR}/opt_parser.cpp"
     "${PROJECT_SOURCE_DIR}/recorderEventHandler.cpp"
     "${PROJECT_SOURCE_DIR}/time_util.cpp")

# 3. Create the executable
add_executable(recorder_local ${SAMPLE_RECORDER_FILES})

# 4. Link the Agora SDK libraries
# Uses the AGORA_SDK_ROOT variable we defined above
target_link_libraries(recorder_local
    "${AGORA_SDK_ROOT}/libagora_rtc_sdk.so"
    "${AGORA_SDK_ROOT}/libaosl.so"
    pthread
    dl
)
