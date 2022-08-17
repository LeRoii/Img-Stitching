cmake_minimum_required(VERSION 2.4)
project(cvtest)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(LIB_PANO_MAJOR 1)
set(LIB_PANO_MINOR 0)
set(LIB_PANO_PATCH 0)

set(NVINFER_VERSION 7)

set(BUILD_testocv OFF CACHE BOOL "build testocv" FORCE)
set(BUILD_cameracuda OFF CACHE BOOL "build nvidia camera v4l2 cuda" FORCE)
set(BUILD_master ON CACHE BOOL "build master, dev app for 8 cams panorama" FORCE)
set(BUILD_master2 OFF CACHE BOOL "build master2, dev app for 2 cams panorama" FORCE)
set(BUILD_replay OFF CACHE BOOL "build replay, debug app for 8 cams panorama" FORCE)
set(BUILD_replay2 OFF CACHE BOOL "build replay2, debug app for 2 cams panorama" FORCE)
set(BUILD_camshow OFF CACHE BOOL "build camshow, 8 camera disp app" FORCE)
set(BUILD_camshow4cam OFF CACHE BOOL "build camshow4cam, 4 camera disp app" FORCE)
set(BUILD_camshow_2 OFF CACHE BOOL "build camshow-2, 2 camera disp app" FORCE)
set(BUILD_panocam OFF CACHE BOOL "build panocam, panorama so" FORCE)
set(BUILD_demo OFF CACHE BOOL "build demo, panorama so test app" FORCE)
set(BUILD_panoapp OFF CACHE BOOL "build panoapp, released panorama app" FORCE)
set(BUILD_slave OFF CACHE BOOL "build slave, 8 panorama slave app" FORCE)
set(BUILD_slaveshow OFF CACHE BOOL "build BUILD_slaveshow, 11 panorama slaveshow app" FORCE)

set(BUILD_sti OFF CACHE BOOL "build sti, opencv stitching test app" FORCE)
set(BUILD_sd OFF CACHE BOOL "build sd, origin opencv stitching app" FORCE)
set(BUILD_mo OFF CACHE BOOL "build mo, modified opencv stitching app" FORCE)
set(BUILD_server OFF CACHE BOOL "build server, udp server" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#add -no-pie to generate executable files, otherwise generate shared library
# set(CMAKE_CXX_FLAGS "-no-pie -g -Wall")
#-------------------------------------------------------------------------------
# CUDA
#-------------------------------------------------------------------------------
find_package(CUDA REQUIRED)
SET(CUDA_SEPARABLE_COMPILATION ON)
message(STATUS "CUDA_VERSION: ${CUDA_VERSION}")
message(STATUS "CUDA_INCLUDE_DIRS：${CUDA_INCLUDE_DIRS}")
message(STATUS "CUDA_LIBRARIES：${CUDA_LIBRARIES}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR：${CMAKE_CURRENT_SOURCE_DIR}")

find_package(CUDNN REQUIRED)
message(STATUS "CUDNN_LIBRARISE:::${CUDNN_LIBRARIES}")
message("-- Found NVINFER: "  ${CUDNN_NVLIB})
set(CUDA_NVCC_FLAGS  ${CUDA_NVCC_FLAGS} --maxrregcount=32; -G;-g;-std=c++11) # nvcc flags

set(CUDIR /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda)
set(COMMONCLS /usr/src/jetson_multimedia_api/samples/common/classes)

set(OpenCV_DIR "/usr/local/share/OpenCV/OpenCVConfig.cmake")
find_package(OpenCV 3 REQUIRED )
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIRS：${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIB_DIR：${OpenCV_LIB_DIR}")
message(STATUS "OpenCV_LIBS：${OpenCV_LIBS}")
message(STATUS "OPENGL_INCLUDE_DIR: ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL lib: ${OPENGL_LIBRARY}")
message(STATUS "GLUT_LIBRARY: ${GLUT_LIBRARY}")

message(STATUS "CUDA_LIBRARIES:::${CUDA_LIBRARIES}")
message(STATUS "CUDA_CUBLAS_LIBRARIES:::${CUDA_CUBLAS_LIBRARIES}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR:::${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CUDNN_LIBRARISE:::${CUDNN_LIBRARIES}")


# add_compile_options(-std=c++14 -g )
add_compile_options(-std=c++14 -pthread)
# add_definitions(-DDEBUG)
add_definitions(-DCAM_IMX390 -DDEV_MODE -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX390 -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX390 -DEXPORT_SO -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX390 -DDEV_MODE -DDISTRIBUTED_STRUCT)
# add_definitions(-DCAM_IMX424 -DDEV_MODE -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX424 -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX424 -DYUYVCAM  -DCENTRIC_STRUCT)
# add_definitions(-DCAM_IMX424)
# add_definitions(-DYUYVCAM -DCAM_IMX424 -DDEV_MODE)
# add_definitions(-DYUYVCAM -DCAM_IMX424)
# add_definitions(-DCAM_IMX390)

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
    /usr/src/jetson_multimedia_api/include
    /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda/
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/
    )

if(${NVINFER_VERSION} EQUAL 7)
    include_directories(include/tkDNN7/)
elseif(${NVINFER_VERSION} EQUAL 8)
    include_directories(include/tkDNN/)
endif()

#-------------------------------------------------------------------------------
# Build Node 
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Build sti 
#-------------------------------------------------------------------------------
if(BUILD_sti)
    add_executable(sti src/sti.cpp )
    target_link_libraries(sti   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
endif()

#-------------------------------------------------------------------------------
# Build sd 
#-------------------------------------------------------------------------------
if(BUILD_sd)
    add_executable(sd src/stitching_detailed.cpp )
    target_link_libraries(sd   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
endif()

#-------------------------------------------------------------------------------
# Build mo 
#-------------------------------------------------------------------------------
if(BUILD_mo)
    add_executable(mo src/stithermo.cpp )
    target_link_libraries(mo   ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} )
endif()

#-------------------------------------------------------------------------------
# Build server 
#-------------------------------------------------------------------------------
if(BUILD_server)
    add_executable(server src/Server.cpp  src/PracticalSocket.cpp)
    target_link_libraries(server 
        ${OpenCV_LIBS})
endif()

#-------------------------------------------------------------------------------
# Build testocv 
#-------------------------------------------------------------------------------
if(BUILD_testocv)
    add_executable(testocv src/testocv.cpp src/imageProcess.cpp src/PracticalSocket.cpp 
    src/nvrenderbase.cpp src/nvrenderAlpha.cpp src/nvrenderbeta.cpp src/stitcherglobal.cpp 
    ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(testocv  -lyaml-cpp ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES} 
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    -lboost_system -lboost_random -lboost_thread -lpthread
    )
    # target_link_libraries(testocv  -lyaml-cpp ${OPENGL_LIBRARY} ${OpenCV_LIBS} )
endif()

#-------------------------------------------------------------------------------
# Build cameracuda 
#-------------------------------------------------------------------------------
if(BUILD_cameracuda)
    add_executable(cameracuda src/camera_v4l2-cuda.cpp ${COMMONCLSFILES})
    target_link_libraries(cameracuda  
        -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
        -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
        ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    )
endif()

#-------------------------------------------------------------------------------
# Build master 
#-------------------------------------------------------------------------------
message(STATUS "before BUILD_master:::::::::: ${BUILD_master}")
if(BUILD_master)
message(STATUS "BUILD_master:::::::::: ${BUILD_master}")

    add_executable(master src/master.cpp src/imageProcess.cpp src/PracticalSocket.cpp 
    src/nvrenderbase.cpp src/nvrenderAlpha.cpp src/nvrender.cpp src/stitcherglobal.cpp 
    ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(master  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS} ${CUDNN_LIBRARIES}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN -lkernels 
    -lboost_system -lboost_random -lboost_thread -lpthread
    )
endif()

#-------------------------------------------------------------------------------
# Build master2 
#-------------------------------------------------------------------------------
if(BUILD_master2)
    add_executable(master2 src/master2.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(master2  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
endif()

#-------------------------------------------------------------------------------
# Build replay 
#-------------------------------------------------------------------------------
if(BUILD_replay)
    add_executable(replay src/replay.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(replay  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
endif()

#-------------------------------------------------------------------------------
# Build replay2 
#-------------------------------------------------------------------------------
if(BUILD_replay2)
    add_executable(replay2 src/replay2.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(replay2  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
endif()

#-------------------------------------------------------------------------------
# Build camshow 
#-------------------------------------------------------------------------------
if(BUILD_camshow)
    add_executable(camshow src/camshow.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES} )
    target_link_libraries(camshow  ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES}  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
endif()


#-------------------------------------------------------------------------------
# Build camshow4cam 
#-------------------------------------------------------------------------------
if(BUILD_camshow4cam)
    add_executable(camshow4cam src/camshow4cam.cpp src/imageProcess.cpp src/PracticalSocket.cpp  src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES} )
    target_link_libraries(camshow4cam  ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES}  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY}
    ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    -lboost_system -lboost_random -lboost_thread -lpthread
    )
endif()
    
#-------------------------------------------------------------------------------
# Build camshow-2 
#-------------------------------------------------------------------------------
if(BUILD_camshow_2)
    add_executable(camshow-2 src/camshow-2.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES} )
    target_link_libraries(camshow-2  ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES}  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
endif()

#-------------------------------------------------------------------------------
# Build panocam
#-------------------------------------------------------------------------------
if(BUILD_panocam)
    add_library(panocam SHARED src/panocam.cpp src/panocamimpl.cpp src/imageProcess.cpp src/PracticalSocket.cpp src/nvrender.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(panocam
        ${OPENGL_LIBRARY} ${OpenCV_LIBS} -lyaml-cpp
        -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
        -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
        -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
    )
    set_target_properties(panocam PROPERTIES VERSION ${LIB_PANO_MAJOR}.${LIB_PANO_MINOR}.${LIB_PANO_PATCH} SOVERSION ${LIB_PANO_MAJOR})
endif()

#-------------------------------------------------------------------------------
# Build demo
#-------------------------------------------------------------------------------
if(BUILD_demo)
    add_executable(demo src/demo.cpp)
    target_link_libraries(demo
        panocam
    )
endif()

#-------------------------------------------------------------------------------
# Build panoapp
#-------------------------------------------------------------------------------
if(BUILD_panoapp)
    add_executable(panoapp src/panoapp.cpp src/nvrender.cpp src/imageProcess.cpp src/context.cpp src/PracticalSocket.cpp src/fsmstate.cpp src/panocamimpl.cpp src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES})
    target_link_libraries(panoapp
        ${OPENGL_LIBRARY} ${OpenCV_LIBS}  ${CUDA_LIBRARIES}
        -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
        -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
        -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -ltkDNN
        -lyaml-cpp
        -lpthread
    )
endif()

#-------------------------------------------------------------------------------
# Build slave
#-------------------------------------------------------------------------------
if(BUILD_slave)
    add_executable(slave src/slave.cpp  src/PracticalSocket.cpp src/stitcherglobal.cpp )
    target_link_libraries(slave 
    ${OPENGL_LIBRARY} ${OpenCV_LIBS} ${CUDA_LIBRARIES}  -lyaml-cpp
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2 -lEGL -lGLESv2 -lX11 
    ${CUDA_LIBRARIES} ${GLUT_LIBRARY} ${OpenCV_LIBS} -lpthread
    )
endif()


#-------------------------------------------------------------------------------
# Build slaveshow
#-------------------------------------------------------------------------------
if(BUILD_slaveshow)
    add_executable(slaveshow src/slaveshow.cpp  src/nvrender.cpp src/imageProcess.cpp src/PracticalSocket.cpp  src/stitcherglobal.cpp ${NVENCODER_SOURCE} ${COMMONCLSFILES}  )
    target_link_libraries(slaveshow 
    -lyaml-cpp 
    ${OPENGL_LIBRARY} 
    ${CUDA_LIBRARIES}
    -lpthread
    -L/usr/lib/aarch64-linux-gnu/ -lv4l2  -lX11 -lEGL
    -L/usr/lib/aarch64-linux-gnu/tegra/ -lnvbuf_utils  
    -L${CMAKE_CURRENT_SOURCE_DIR}/cfg/ -lkernels -ltkDNN
    ${OpenCV_LIBS} 
    )
endif()
