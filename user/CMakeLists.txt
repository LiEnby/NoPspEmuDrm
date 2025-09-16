cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(NoPspEmuDrm_user)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DLOGGING_ENABLED=1")

add_executable(${PROJECT_NAME}
	crypto/aes.c
	crypto/amctrl.c
	crypto/bn.c
	crypto/ec.c
	crypto/kirk_engine.c
	crypto/sha1.c

	PspEmu.c
	SceShell.c

	PspNpDrm.c
	Crypto.c
	Io.c
	Pbp.c
	Main.c
)

include_directories(${CMAKE_SOURCE_DIR}/user)
include_directories(${CMAKE_SOURCE_DIR}/kern)


target_link_libraries(${PROJECT_NAME}
	taihen_stub
	SceLibKernel_stub
	SceCompat_stub
	SceLibc_stub_weak
	SceNpDrm_stub
	SceRegistryMgr_stub
	SceAppMgr_stub
	SceDisplay_stub
	SceIofilemgr_stub
	SceKernelModulemgr_stub
	SceRtc_stub
	SceVshBridge_stub
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml
)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.yml)
