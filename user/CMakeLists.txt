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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")


add_executable(NoPspEmuDrm_user
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

target_link_libraries(NoPspEmuDrm_user
	taiHEN_stub
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



set_target_properties(NoPspEmuDrm_user
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(NoPspEmuDrm_user.suprx NoPspEmuDrm_user CONFIG exports.yml UNSAFE)
vita_create_stubs(stubs NoPspEmuDrm_user ${CMAKE_SOURCE_DIR}/exports.yml)
