cmake_minimum_required(VERSION 3.15)
project(episteme CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(DEFAULT_NET "1024_simple_v3.bin" CACHE STRING "Default network file")
set(EVALFILE "${DEFAULT_NET}" CACHE STRING "Evaluation file to use")
set(NETS_REPO "https://github.com/aletheiaaaaa/episteme-nets")
get_filename_component(NET_FILENAME "${DEFAULT_NET}" NAME)
set(NET_URL "${NETS_REPO}/releases/latest/download/${NET_FILENAME}")

if(NOT DEFINED ARCH)
  set(_PROBE "${CMAKE_BINARY_DIR}/arch_probe.cpp")
  file(WRITE "${_PROBE}" "")
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -march=native -dM -E -x c++ "${_PROBE}"
    OUTPUT_VARIABLE _NATIVE_MACROS
    ERROR_QUIET
  )
  if(_NATIVE_MACROS MATCHES "__AVX512VNNI__")
    set(DETECTED_ARCH "avx512_vnni")
  elseif(_NATIVE_MACROS MATCHES "__AVX2__")
    set(DETECTED_ARCH "avx2")
  elseif(_NATIVE_MACROS MATCHES "__SSSE3__")
    set(DETECTED_ARCH "ssse3")
  else()
    set(DETECTED_ARCH "generic")
  endif()
else()
  set(DETECTED_ARCH "${ARCH}")
endif()

if(DETECTED_ARCH STREQUAL "avx512_vnni")
  set(ARCH_FLAGS "-mavx512f" "-mavx512bw" "-mavx512dq" "-mavx512vl" "-mavx512vnni")
  set(ARCH_DEFS "USE_AVX512" "USE_VNNI")
  message(STATUS "Building with AVX-512 + VNNI support")
elseif(DETECTED_ARCH STREQUAL "avx2")
  set(ARCH_FLAGS "-mavx2" "-mfma")
  set(ARCH_DEFS "USE_AVX2")
  message(STATUS "Building with AVX2 support")
elseif(DETECTED_ARCH STREQUAL "ssse3")
  set(ARCH_FLAGS "-mssse3")
  set(ARCH_DEFS "USE_SSSE3")
  message(STATUS "Building with SSSE3 support")
else()
  set(ARCH_FLAGS "")
  set(ARCH_DEFS "")
  message(STATUS "Building with generic SSE/SSE2 support")
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(episteme ${SOURCES})

target_compile_options(episteme PRIVATE ${ARCH_FLAGS})

target_compile_definitions(episteme PRIVATE
  ${ARCH_DEFS}
  EVALFILE="${CMAKE_SOURCE_DIR}/${EVALFILE}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(episteme PRIVATE -g -O0)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(episteme PRIVATE -fsanitize=address,undefined)
    target_link_options(episteme PRIVATE -fsanitize=address,undefined)
  endif()
else()
  target_compile_options(episteme PRIVATE -O3)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(episteme PRIVATE -flto=auto)
    target_link_options(episteme PRIVATE -flto=auto)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(episteme PRIVATE -flto)
    target_link_options(episteme PRIVATE -flto)
  endif()
endif()

if(MINGW)
  target_link_libraries(episteme PRIVATE stdc++exp)
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}")
  message(STATUS "Network file ${DEFAULT_NET} not found. Downloading...")

  find_program(CURL_EXECUTABLE curl)
  find_program(WGET_EXECUTABLE wget)

  if(CURL_EXECUTABLE)
    execute_process(
      COMMAND ${CURL_EXECUTABLE} -L -o "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}" "${NET_URL}"
      RESULT_VARIABLE DOWNLOAD_RESULT
    )
  elseif(WGET_EXECUTABLE)
    execute_process(
      COMMAND ${WGET_EXECUTABLE} -O "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}" "${NET_URL}"
      RESULT_VARIABLE DOWNLOAD_RESULT
    )
  elseif(WIN32)

    execute_process(
      COMMAND powershell -NoProfile -ExecutionPolicy Bypass -Command
        "Invoke-WebRequest -Uri '${NET_URL}' -OutFile '${CMAKE_SOURCE_DIR}/${DEFAULT_NET}' -UseBasicParsing"
      RESULT_VARIABLE DOWNLOAD_RESULT
    )
  else()
    message(FATAL_ERROR
      "Neither curl nor wget found. Please install one or download manually:\n  ${NET_URL}")
  endif()

  if(DOWNLOAD_RESULT EQUAL 0)
    message(STATUS "Downloaded ${DEFAULT_NET} successfully")
  else()
    message(FATAL_ERROR "Failed to download ${NET_URL}")
  endif()
endif()

add_custom_target(download-net
  COMMAND ${CMAKE_COMMAND} -E remove -f "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}"
  COMMAND ${CMAKE_COMMAND} -E echo "Downloading network file..."
  COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}
  COMMENT "Re-downloading network file"
)

add_custom_target(clean-all
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  COMMAND ${CMAKE_COMMAND} -E remove -f "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}"
  COMMENT "Cleaning all including network file"
)

add_custom_target(rebuild
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
  COMMENT "Clean and rebuild"
)

add_custom_target(rebuild-all
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  COMMAND ${CMAKE_COMMAND} -E remove -f "${CMAKE_SOURCE_DIR}/${DEFAULT_NET}"
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
  COMMENT "Clean all and rebuild"
)

add_custom_target(show-arch
  COMMAND ${CMAKE_COMMAND} -E echo "Detected architecture: ${DETECTED_ARCH}"
  COMMAND ${CMAKE_COMMAND} -E echo "Architecture flags: ${ARCH_FLAGS}"
  COMMAND ${CMAKE_COMMAND} -E echo "Architecture defines: ${ARCH_DEFS}"
  COMMENT "Show detected architecture and compiler flags"
)
