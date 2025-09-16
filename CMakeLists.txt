cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(NoPspEmuDrm)
include("${VITASDK}/share/vita.cmake" REQUIRED)

add_subdirectory(kern)
add_subdirectory(user)

add_custom_target(${PROJECT_NAME}
	ALL
	DEPENDS NoPspEmuDrm_user.suprx-self NoPspEmuDrm_kern.skprx-self
	COMMAND mv ${CMAKE_BINARY_DIR}/kern/NoPspEmuDrm_kern.skprx .
	COMMAND mv ${CMAKE_BINARY_DIR}/user/NoPspEmuDrm_user.suprx .
)

