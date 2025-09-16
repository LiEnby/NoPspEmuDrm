cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(NoPspEmuDrm_kern)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DLOGGING_ENABLED=1")

add_executable(${PROJECT_NAME}
  Main.c
  EcPatch.c
  RifPatch.c
  CompatPatch.c
  EbootSigPatch.c
)

include_directories(${CMAKE_SOURCE_DIR}/user)
include_directories(${CMAKE_SOURCE_DIR}/kern)

target_link_libraries(${PROJECT_NAME}
  taihenForKernel_stub
  SceIofilemgrForDriver_stub
  SceNpDrmForDriver_stub
  SceRegMgrForDriver_stub
  SceSblSsMgrForDriver_stub
  SceSysclibForDriver_stub
  SceDebugForDriver_stub
  SceThreadmgrForDriver_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml
)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml
  KERNEL
)