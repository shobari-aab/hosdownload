cmake_minimum_required(VERSION 2.4)
project(DefaultSamples)

include_directories(
  "${PROJECT_SOURCE_DIR}"
)

# Build sample_receive_mixed_audio
file(GLOB SAMPLE_RECORDER_FILES
     "${PROJECT_SOURCE_DIR}/sample_recorder.cpp"
     "${PROJECT_SOURCE_DIR}/recorderEventHandler.cpp"
     "${PROJECT_SOURCE_DIR}/parser/parser.cpp"
     "${PROJECT_SOURCE_DIR}/parser/cJSON.c")
add_executable(sample_recorder ${SAMPLE_RECORDER_FILES})