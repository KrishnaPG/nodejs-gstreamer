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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(PkgConfig REQUIRED)

# Platform-specific GStreamer setup
if(WIN32)
    # Windows (D3D11)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)
else()
    # Linux (VA-API)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-vaapi-1.0)
    add_definitions(-DUSE_VAAPI)
endif()

# Node.js and N-API setup
add_compile_definitions(NAPI_VERSION=9)

##############
# Node.js addon
##
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
    ${GSTREAMER_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api" 
    "${CMAKE_SOURCE_DIR}/node_modules/node-api-headers/include" 
    "${CMAKE_SOURCE_DIR}/src" )

# Linker flags
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_JS_LIB}
    ${GSTREAMER_LIBRARIES}
)

# Platform-specific optimizations
if (CMAKE_BUILD_TYPE EQUAL "Release")
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /fp:fast /arch:AVX2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native -ffast-math)
endif()
endif()

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

#################
## Test Executable
add_executable(test_gstreamer_pipeline test/test_gstreamer_pipeline.cpp src/gstreamer-pipeline.cpp )

target_include_directories(test_gstreamer_pipeline PUBLIC "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api" "${CMAKE_SOURCE_DIR}/node_modules/node-api-headers/include" "${CMAKE_SOURCE_DIR}/src" ${GSTREAMER_INCLUDE_DIRS} )

target_link_libraries(test_gstreamer_pipeline ${GSTREAMER_LIBRARIES})

set_target_properties(test_gstreamer_pipeline PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/Release )
