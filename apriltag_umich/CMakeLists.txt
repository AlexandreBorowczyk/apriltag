cmake_minimum_required(VERSION 2.8)
project(apriltag_umich)

set(USE_ROS TRUE)
include(cmake/apriltag_umich-extras.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package (Threads)

if(USE_ROS)
    find_package(catkin REQUIRED COMPONENTS roslib)
    find_package(OpenCV REQUIRED)
    catkin_package(
        INCLUDE_DIRS include src src/common
        LIBRARIES ${PROJECT_NAME}
        CFG_EXTRAS apriltag_umich-extras.cmake
        )
else()
    message(STATUS "Source: " ${PROJECT_SOURCE_DIR})
    message(STATUS "Binary: " ${PROJECT_BINARY_DIR})
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    link_directories(${PROJECT_SOURCE_DIR}/lib)
endif()

include_directories(
    include
    src
    src/common
    )

file(GLOB C_FILES
    ${PROJECT_SOURCE_DIR}/src/*.c
    ${PROJECT_SOURCE_DIR}/src/common/*.c)
list(REMOVE_ITEM C_FILES ${PROJECT_SOURCE_DIR}/src/apriltag_demo.c)

add_library(${PROJECT_NAME} SHARED ${C_FILES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} m)

#if(CATKIN_ENABLE_TESTING)
if(FALSE)
    include_directories(${catkin_INCLUDE_DIRS})
    catkin_add_gtest(test_${PROJECT_NAME} test/test_${PROJECT_NAME}.cpp)
    target_link_libraries(test_${PROJECT_NAME}
        ${PROJECT_NAME}
        ${catkin_LIBRARIES}
        ${OpenCV_LIBRARIES}
        )
endif()
