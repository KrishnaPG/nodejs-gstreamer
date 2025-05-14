cmake_minimum_required(VERSION 3.12)

if(WIN32)
set(ENABLE_VCPKG ON CACHE BOOL "Use VCPkg for package finding")
  if(ENABLE_VCPKG)
    set(VCPKG_ROOT_DIR $ENV{VCPKG_ROOT} CACHE PATH "VCPkg Root Folder Path (where vcpkg.exe is located)")
    if("${VCPKG_ROOT_DIR}" STREQUAL "")
      message(FATAL_ERROR "\nVCPKG_ROOT is not found. \nChoose the VCPKG_ROOT_DIR path to proceed, or turn off the ENABLE_VCPKG option to manually discover the packages.")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake")
  endif()
endif(WIN32)

project(gstzerocopy)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Node.js and N-API setup
add_compile_definitions(-DNAPI_VERSION=9)

# GStreamer dependencies
find_package(PkgConfig REQUIRED)

# Platform-specific GStreamer setup
if(WIN32)
    # Windows (D3D11)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-d3d11-1.0)
    add_definitions(-DUSE_D3D11)
else()
    # Linux (VA-API)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-vaapi-1.0)
    add_definitions(-DUSE_VAAPI)
endif()

include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

# Node.js addon
add_library(${PROJECT_NAME} SHARED
    src/gstreamer-pipeline.cpp
     ${CMAKE_JS_SRC}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/Release
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_JS_INC}
    "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api" 
    "${CMAKE_SOURCE_DIR}/node_modules/node-api-headers/include" 
    "${CMAKE_SOURCE_DIR}/src" )

# Linker flags
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_JS_LIB}
    ${GSTREAMER_LIBRARIES}
)

# Platform-specific optimizations
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /fp:fast /arch:AVX2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native -ffast-math)
endif()

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

# Install target
