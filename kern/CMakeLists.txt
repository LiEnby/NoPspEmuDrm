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

add_executable(NoPspEmuDrm_kern
  Main.c
  EcPatch.c
  RifPatch.c
  CompatPatch.c
  EbootSigPatch.c
)

target_link_libraries(NoPspEmuDrm_kern
  taihenForKernel_stub
  SceIofilemgrForDriver_stub
  SceNpDrmForDriver_stub
  SceRegMgrForDriver_stub
  SceSblSsMgrForDriver_stub
  SceSysclibForDriver_stub
  SceDebugForDriver_stub
  SceThreadmgrForDriver_stub
)

vita_create_self(NoPspEmuDrm_kern.skprx NoPspEmuDrm_kern CONFIG exports.yml UNSAFE)