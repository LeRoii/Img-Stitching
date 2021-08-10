cmake_minimum_required(VERSION 2.4)
project(cvtest)


if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(CUDA REQUIRED)
message(STATUS "CUDA版本: ${CUDA_VERSION}")
message(STATUS "    头文件目录：${CUDA_INCLUDE_DIRS}")
message(STATUS "    库文件列表：${CUDA_LIBRARIES}")
set(CUDA_NVCC_FLAGS -G;-g;-std=c++11) # nvcc flags
include_directories(
    ${CUDA_INCLUDE_DIRS}
    /opt/MVS/include)


message("msg CMAKE_PREFIX_PATH:::${CMAKE_PREFIX_PATH}")


set(OpenCV_DIR "/usr/local/share/OpenCV/OpenCVConfig.cmake")
find_package( OpenCV REQUIRED )

message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "    头文件目录：${OpenCV_INCLUDE_DIRS}")
message(STATUS "    库文件目录：${OpenCV_LIB_DIR}")
message(STATUS "    库文件列表：${OpenCV_LIBS}")

add_compile_options(-std=c++11 -g)
add_definitions(-DDEBUG)

add_executable(cvtest cvstitching.cpp)
add_executable(camera camera.cpp)
target_link_libraries(cvtest ${OpenCV_LIBS} ${CUDA_LIBRARIES})
target_link_libraries(camera ${OpenCV_LIBS} ${CUDA_LIBRARIES} 
 -L/opt/MVS/lib/64 MvCameraControl.so)

# add_executable(cvtest cvtest.cpp)
# target_link_libraries( cvtest ${OpenCV_LIBS} )