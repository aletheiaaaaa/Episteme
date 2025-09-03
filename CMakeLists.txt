cmake_minimum_required(VERSION 3.10.0)
project(episteme)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-mavx2 -g -fsanitize=address,undefined,leak")
set(CMAKE_CXX_FLAGS_RELEASE "-mavx2 -O3 -flto=auto")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 23)
set(SRC "${CMAKE_SOURCE_DIR}/src")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Network file configuration
set(EVAL_BIN "${CMAKE_SOURCE_DIR}/256_v0_05.bin")
set(NETS_REPO "https://github.com/aletheiaaaaa/episteme-nets")
set(NET_FILENAME "256_v0_05.bin")
set(NET_URL "${NETS_REPO}/releases/latest/download/${NET_FILENAME}")

# Function to download the network file
function(download_network_file)
    message(STATUS "Network file ${EVAL_BIN} not found. Downloading...")
    
    find_program(WGET_EXECUTABLE wget)
    find_program(CURL_EXECUTABLE curl)
    
    if(WGET_EXECUTABLE)
        execute_process(
            COMMAND ${WGET_EXECUTABLE} -O ${EVAL_BIN} ${NET_URL}
            RESULT_VARIABLE DOWNLOAD_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
    elseif(CURL_EXECUTABLE)
        execute_process(
            COMMAND ${CURL_EXECUTABLE} -L -o ${EVAL_BIN} ${NET_URL}
            RESULT_VARIABLE DOWNLOAD_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
    else()
        message(FATAL_ERROR 
            "Neither wget nor curl found. Please install one of them or "
            "download ${NET_URL} manually to ${EVAL_BIN}")
    endif()
    
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download ${NET_URL}")
    endif()
    
    message(STATUS "Downloaded ${EVAL_BIN} successfully")
endfunction()

# Check if network file exists, download if not
if(NOT EXISTS ${EVAL_BIN})
    download_network_file()
endif()

# Verify the file exists after potential download
if(NOT EXISTS ${EVAL_BIN})
    message(FATAL_ERROR "Network file ${EVAL_BIN} is missing and could not be downloaded")
endif()

add_executable(episteme
    "${SRC}/engine/chess/position.cpp" 
    "${SRC}/engine/chess/move.cpp" 
    "${SRC}/engine/chess/movegen.cpp" 
    "${SRC}/engine/chess/perft.cpp" 
    "${SRC}/engine/chess/zobrist.cpp" 
    "${SRC}/engine/evaluation/evaluate.cpp" 
    "${SRC}/engine/evaluation/nnue.cpp" 
    "${SRC}/engine/search/search.cpp" 
    "${SRC}/engine/search/ttable.cpp"
    "${SRC}/engine/uci/uci.cpp" 
    "${SRC}/utils/datagen.cpp"
    "${SRC}/utils/format.cpp"
    "${SRC}/main.cpp"
)

target_compile_definitions(episteme PRIVATE EVALFILE="${EVAL_BIN}")

# Add the network file as a dependency so it gets checked on each build
add_custom_command(
    OUTPUT ${EVAL_BIN}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/download_net.cmake
    COMMENT "Checking/downloading network file"
)

# Create the download script
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/download_net.cmake "
set(EVAL_BIN \"${EVAL_BIN}\")
set(NET_URL \"${NET_URL}\")

if(NOT EXISTS \${EVAL_BIN})
    message(STATUS \"Network file \${EVAL_BIN} not found. Downloading...\")
    
    find_program(WGET_EXECUTABLE wget)
    find_program(CURL_EXECUTABLE curl)
    
    if(WGET_EXECUTABLE)
        execute_process(
            COMMAND \${WGET_EXECUTABLE} -O \${EVAL_BIN} \${NET_URL}
            RESULT_VARIABLE DOWNLOAD_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
    elseif(CURL_EXECUTABLE)
        execute_process(
            COMMAND \${CURL_EXECUTABLE} -L -o \${EVAL_BIN} \${NET_URL}
            RESULT_VARIABLE DOWNLOAD_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
    else()
        message(FATAL_ERROR 
            \"Neither wget nor curl found. Please install one of them or \"
            \"download \${NET_URL} manually to \${EVAL_BIN}\")
    endif()
    
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR \"Failed to download \${NET_URL}\")
    endif()
    
    message(STATUS \"Downloaded \${EVAL_BIN} successfully\")
endif()
")

# Add custom targets for network management
add_custom_target(download-net
    COMMAND ${CMAKE_COMMAND} -E remove -f ${EVAL_BIN}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/download_net.cmake
    COMMENT "Force downloading latest network file"
)

add_custom_target(clean-net
    COMMAND ${CMAKE_COMMAND} -E remove -f ${EVAL_BIN}
    COMMENT "Remove network file"
)