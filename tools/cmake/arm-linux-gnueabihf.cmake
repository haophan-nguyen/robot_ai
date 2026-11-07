set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(AI_ROBOT_ARMHF_SYSROOT "" CACHE PATH "Bookworm armhf sysroot")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES AI_ROBOT_ARMHF_SYSROOT)
if(NOT IS_ABSOLUTE "${AI_ROBOT_ARMHF_SYSROOT}")
    message(FATAL_ERROR "Set AI_ROBOT_ARMHF_SYSROOT to an absolute sysroot path.")
endif()

set(CMAKE_SYSROOT "${AI_ROBOT_ARMHF_SYSROOT}")
set(_target_lib "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf")
set(_target_runtime "${CMAKE_SYSROOT}/lib/arm-linux-gnueabihf")
foreach(_required IN ITEMS
    "${CMAKE_SYSROOT}/lib/ld-linux-armhf.so.3"
    "${_target_lib}/Scrt1.o"
    "${_target_lib}/libc.so"
    "${_target_lib}/libstdc++.so"
    "${_target_lib}/libgcc_s.so"
)
    if(NOT EXISTS "${_required}")
        message(FATAL_ERROR "Incomplete armhf sysroot: missing ${_required}")
    endif()
endforeach()

set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")

# Select the target's startup files and runtimes ahead of GCC's bundled sysroot.
set(_link_flags
    "-B\"${_target_lib}/\" -L\"${_target_lib}\" -L\"${_target_runtime}\" \"-Wl,-rpath-link,${_target_lib}:${_target_runtime}\"")
foreach(_kind IN ITEMS EXE SHARED MODULE)
    set(CMAKE_${_kind}_LINKER_FLAGS_INIT "${_link_flags}")
endforeach()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${_target_lib}/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
