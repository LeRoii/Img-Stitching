cmake_minimum_required (VERSION 3.8 FATAL_ERROR)
project (GpuBTree)

find_package(CUDA 8.0 REQUIRED)

option(CMAKE_VERBOSE_MAKEFILE ON)
option(DGTEST,  "DGTEST"  ON)

set(CUDA_NVCC_FLAGS -std=c++11)
set (CMAKE_CXX_STANDARD 11)

set(CUDIR /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda)
set(COMMONCLS /usr/src/jetson_multimedia_api/samples/common/classes)

if (CUDA_VERBOSE_PTXAS)
  set(VERBOSE_PTXAS --ptxas-options=-v)
endif (CUDA_VERBOSE_PTXAS)

#set(CMAKE_BUILD_TYPE "Release")

include_directories(
    /usr/src/jetson_multimedia_api/include
    /usr/src/jetson_multimedia_api/samples/common/algorithm/cuda/
    )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(GENCODE_SM71
  -gencode=arch=compute_72,code=sm_72 -gencode=arch=compute_72,code=compute_72)

set(CUFILES
    ${CUDIR}/NvCudaProc.cpp
    ${CUDIR}/NvAnalysis.cu)

set(NVFILES
  ${COMMONCLS}/NvBuffer.cpp
  ${COMMONCLS}/NvElement.cpp
  ${COMMONCLS}/NvElementProfiler.cpp
  ${COMMONCLS}/NvEglRenderer.cpp
  ${COMMONCLS}/NvJpegDecoder.cpp
  ${COMMONCLS}/NvLogging.cpp
  )

cuda_add_library(cameracu SHARED
	${CUFILES}
	OPTIONS ${GENCODE_SM71} ${VERBOSE_PTXAS})

# add_library(nvsrc SHARED
#   ${NVFILES}
# )

target_link_libraries(cameracu ${CUDA_LIBRARIES}
-L/usr/local/lib64/ -lcuda -ldl
-L/usr/local/cuda-10.2/targets/aarch64-linux/lib/ -lcudart

  )

