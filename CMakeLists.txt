cmake_minimum_required(VERSION 2.4)
project(cvtest)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(CUDA REQUIRED)
message(STATUS "CUDA版本: ${CUDA_VERSION}")
message(STATUS "    头文件目录：${CUDA_INCLUDE_DIRS}")
message(STATUS "    库文件列表：${CUDA_LIBRARIES}")
message(STATUS "  CMAKE_CURRENT_SOURCE_DIR：${CMAKE_CURRENT_SOURCE_DIR}")

set(CUDA_NVCC_FLAGS -G;-g;-std=c++11) # nvcc flags

add_subdirectory(cudasrc)
include_directories(cudasrc)

include_directories(
    ${CUDA_INCLUDE_DIRS}
    /opt/MVS/include)

set(OpenCV_DIR "/usr/local/share/OpenCV/OpenCVConfig.cmake")
find_package( OpenCV 3 REQUIRED )
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)


message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "    头文件目录：${OpenCV_INCLUDE_DIRS}")
message(STATUS "    库文件目录：${OpenCV_LIB_DIR}")
message(STATUS "    库文件列表：${OpenCV_LIBS}")
message(STATUS "OPENGL_INCLUDE_DIR: ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL lib: ${OPENGL_LIBRARY}")
message(STATUS "GLUT_LIBRARY: ${GLUT_LIBRARY}")

add_compile_options(-std=c++11 -g)
# add_definitions(-DDEBUG)

add_executable(cvtest cvstitching.cpp)
# add_executable(camera camera.cpp)
add_executable(main main.cpp )
add_executable(xavier xavier.cpp PracticalSocket.cpp )
add_executable(sti sti.cpp )
add_executable(sd stitching_detailed.cpp )
add_executable(mo stithermo.cpp )
add_executable(testocv testocv.cpp )
# add_executable(calib calib.cpp )
target_link_libraries(cvtest yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
target_link_libraries(main yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
target_link_libraries(xavier  yuyv2rgb ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(sti   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(sd   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(mo   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(testocv   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(calib   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(camera ${OpenCV_LIBS} ${CUDA_LIBRARIES} 
#  -L/opt/MVS/lib/64 MvCameraControl.so)
