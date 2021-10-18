cmake_minimum_required(VERSION 2.4)
project(cvtest)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)
#-------------------------------------------------------------------------------
# CUDA
#-------------------------------------------------------------------------------
find_package(CUDA REQUIRED)
SET(CUDA_SEPARABLE_COMPILATION ON)
message(STATUS "CUDA版本: ${CUDA_VERSION}")
message(STATUS "    头文件目录：${CUDA_INCLUDE_DIRS}")
message(STATUS "    库文件列表：${CUDA_LIBRARIES}")
message(STATUS "  CMAKE_CURRENT_SOURCE_DIR：${CMAKE_CURRENT_SOURCE_DIR}")

set(CUDA_NVCC_FLAGS  ${CUDA_NVCC_FLAGS} --maxrregcount=32; -G;-g;-std=c++11) # nvcc flags

set(CUDIR /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda)
set(COMMONCLS /usr/src/jetson_multimedia_api/samples/common/classes)


add_subdirectory(cudasrc)
include_directories(cudasrc)

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

message(STATUS "CUDA_LIBRARIES:::${CUDA_LIBRARIES}")
message(STATUS "CUDA_CUBLAS_LIBRARIES:::${CUDA_CUBLAS_LIBRARIES}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR:::${CMAKE_CURRENT_SOURCE_DIR}")

add_compile_options(-std=c++11 -g)
# add_definitions(-DDEBUG)

cuda_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CUDA_INCLUDE_DIRS} )

set(NVENCODER_SOURCE  src/nvencoder/jetsonEncoder.cpp
        src/nvencoder/udp_publisher.cpp)

set(COMMONCLSFILES
    ${COMMONCLS}/NvBuffer.cpp
    ${COMMONCLS}/NvElement.cpp
    ${COMMONCLS}/NvElementProfiler.cpp
    ${COMMONCLS}/NvEglRenderer.cpp
    ${COMMONCLS}/NvLogging.cpp
    ${COMMONCLS}/NvV4l2Element.cpp
    ${COMMONCLS}/NvV4l2ElementPlane.cpp
    ${COMMONCLS}/NvVideoEncoder.cpp
    ${COMMONCLS}/NvVideoDecoder.cpp
    ${COMMONCLS}/NvLogging.cpp)

include_directories(
    ${CUDA_INCLUDE_DIRS}
    include
    include/nvencoder 
    include/tkDNN/
    /usr/src/jetson_multimedia_api/include
    /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda/
    )

#-------------------------------------------------------------------------------
# Build Node 
#-------------------------------------------------------------------------------

add_executable(${PROJECT_NAME} src/imageProcess.cpp src/xavier.cpp src/PracticalSocket.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES} )

add_executable(camshow src/camshow.cpp src/PracticalSocket.cpp  ${NVENCODER_SOURCE} ${COMMONCLSFILES} )
add_executable(sti src/sti.cpp )
add_executable(sd src/stitching_detailed.cpp )
add_executable(mo src/stithermo.cpp )
add_executable(testocv src/testocv.cpp src/imageProcess.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
add_executable(cameracuda src/camera_v4l2-cuda.cpp ${COMMONCLSFILES})
add_executable(master src/master.cpp src/imageProcess.cpp src/PracticalSocket.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})

target_link_libraries(${PROJECT_NAME}
  ${tkdnn_LIBS}
  -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
  ${OpenCV_LIBS}
  ${GLUT_LIBRARY}
  ${CUDA_LIBRARIES}
   ${OPENGL_LIBRARY}
  v4l2
  yuyv2rgb
 )

 target_link_libraries(master  
 -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
 -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
  ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
  -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
  )

# target_link_libraries(cvtest yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
# target_link_libraries(main yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
# target_link_libraries(xavier  yuyv2rgb ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(sti   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(sd   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(mo   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})
# target_link_libraries(testocv   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GLUT_LIBRARY})

target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/cfg/libtkDNN.so")
target_link_libraries(${PROJECT_NAME} "/usr/local/cuda/lib64/libcudart.so")
target_link_libraries(${PROJECT_NAME} "/usr/local/cuda/lib64/libnvrtc.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/tegra/libcuda.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/tegra/libnvbuf_utils.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/tegra/libnvjpeg.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/tegra/libnvosd.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/tegra/libdrm.so.2")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libnvparsers.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libnvinfer.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libX11.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libEGL.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libGLESv2.so")
target_link_libraries(${PROJECT_NAME} "/usr/lib/aarch64-linux-gnu/libv4l2.so")
target_link_libraries(${PROJECT_NAME}  "/usr/lib/aarch64-linux-gnu/libpthread.so")

# add_executable(calib calib.cpp )
# target_link_libraries(cvtest yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
# target_link_libraries(main yuyv2rgb ${OpenCV_LIBS} ${CUDA_LIBRARIES})
target_link_libraries(camshow  yuyv2rgb ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} 
-L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
 -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
  ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
  )
target_link_libraries(sti   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
target_link_libraries(sd   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
target_link_libraries(mo   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
target_link_libraries(testocv   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} 
-L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
 -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
-L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN)

target_link_libraries(cameracuda  
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
     ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
     )
