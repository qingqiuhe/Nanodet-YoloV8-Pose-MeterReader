cmake_minimum_required(VERSION 3.11)
project(meterReader)

set(project_name meterReader)
set(project_root ${PROJECT_SOURCE_DIR})
set(project_src ${project_root}/src)
set(project_include ${project_root}/include)
set(EXECUTABLE_OUTPUT_PATH ${project_root})

option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-std=c++11)
add_definitions(-pthread)

find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message("OpenMP found")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

find_package(OpenCV REQUIRED)
find_package(ncnn REQUIRED)
if(NOT TARGET ncnn)
    message(WARNING "ncnn NOT FOUND! Please set ncnn_DIR environment variable")
else()
    message("ncnn found")
endif()

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Add FetchContent module for external dependencies
include(FetchContent)

# httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)


include_directories(
    ${project_include}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${httplib_SOURCE_DIR}
    ${nlohmann_json_SOURCE_DIR}/include
)

add_executable(
    meterReader
    src/main.cpp
    src/yolov8_pose.cpp
    src/nanodet.cpp
)

target_link_libraries(
    meterReader
    ${OpenCV_LIBS}
    ncnn
)

add_executable(
    meter_reader_api
    src/web_api.cpp
    src/yolov8_pose.cpp
    src/nanodet.cpp
)

target_link_libraries(
    meter_reader_api
    ${OpenCV_LIBS}
    ncnn
)