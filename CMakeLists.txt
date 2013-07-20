project(kmssnake)
cmake_minimum_required(VERSION 2.8)
include_directories(src)
include_directories(/usr/include/libdrm)
file(GLOB_RECURSE SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_executable(${PROJECT_NAME} ${SRC_LIST})


target_link_libraries(${PROJECT_NAME} drm EGL gbm GL)
