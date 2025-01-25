cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(ExternalProject)

project (node-version-weaver)

include_directories(${CMAKE_SOURCE_DIR}/node_modules/node-addon-api)
include_directories(${CMAKE_JS_INC})

file(GLOB SOURCE_FILES "node-semver.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Define NAPI_VERSION
add_definitions(-DNAPI_VERSION=9)

ExternalProject_Add(version_weaver
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/version_weaver
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)
add_dependencies(${PROJECT_NAME} version_weaver)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/version_weaver/include)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/version_weaver/lib/libversion_weaver.a)

function (add_custom_command_with_target _targetName _output firstDep)
    add_custom_command(
        OUTPUT ${_output}
        COMMAND sed "s/__FUNCTION__/${FUNCTION}/" ${firstDep} > ${_output}
        DEPENDS ${firstDep} ${ARGN}
    )
    add_custom_target(${_targetName} ALL DEPENDS ${_output})
endfunction()

set(FUNCTIONS coerce parse valid)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/functions
    COMMAND mkdir ${CMAKE_BINARY_DIR}/functions
    VERBATIM
)
foreach(FUNCTION ${FUNCTIONS})
  add_custom_command_with_target(${FUNCTION} ${CMAKE_BINARY_DIR}/functions/${FUNCTION}.js ${CMAKE_SOURCE_DIR}/templates/functions.template ${CMAKE_BINARY_DIR}/functions)
endforeach()

