cmake_minimum_required(VERSION 3.11)

project(routio CXX)
set(PROJECT_VERSION 1.0.0)

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_APPS "Build CLI programs" ON)
option(BUILD_PYTHON "Build Python wrapper (requires pybind11 and numpy)" OFF)
option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_DEBUG "Enable debug output" OFF)
option(BUILD_TESTS "Build tests" OFF)

find_package(OpenCV QUIET COMPONENTS core videoio highgui)

if (OpenCV_FOUND)
    set(BUILD_OPENCV ON)
    include_directories(${OpenCV_INCLUDE_DIR})
endif()

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    endif ()
else ()
    set (CMAKE_CXX_STANDARD 20)
endif ()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto")
endif()

if(BUILD_DEBUG)
	add_definitions(-DROUTIO_DEBUG)
    add_definitions(-DROUTIO_SOURCE_COMPILE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/src/")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set(LIBRARY_SRC 
    src/array.cpp
    src/loop.cpp
    src/message.cpp
    src/client.cpp
    src/server.cpp
    src/routing.cpp
    src/datatypes.cpp
    src/debug.cpp
)

set(LIBRARY_HEADERS 
    include/routio/loop.h
    include/routio/client.h
    include/routio/camera.h
    include/routio/server.h
    include/routio/routing.h
    include/routio/message.h
    include/routio/datatypes.h
    include/routio/helpers.h
    include/routio/array.h
)

# routio shared library
add_library(routio SHARED ${LIBRARY_SRC})

target_compile_options(routio PUBLIC "-pthread")
target_link_libraries(routio PUBLIC "pthread")

if (BUILD_OPENCV)
target_compile_definitions(routio PUBLIC "BUILD_OPENCV")
TARGET_LINK_LIBRARIES(routio PUBLIC ${OpenCV_LIBS})
endif()

#target_compile_options(routio_static PUBLIC "-pthread" "-fPIC")
#target_link_libraries(routio_static PUBLIC "pthread")

set_target_properties(routio PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)

install(TARGETS routio EXPORT routio_targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${LIBRARY_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/routio)

# CMake config file
include(CMakePackageConfigHelpers)

set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(routio-config.cmake.in
    ${PROJECT_BINARY_DIR}/routio-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/routio
    PATH_VARS LIB_INSTALL_DIR INCLUDE_INSTALL_DIR)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/routio-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/routio-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/routio-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/routio)

install(
    EXPORT routio_targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/routio
    FILE routio-targets.cmake
)

if(BUILD_APPS)
    add_executable(routio_router src/apps/router.cpp)
    target_link_libraries(routio_router routio)
    install(TARGETS routio_router DESTINATION ${CMAKE_INSTALL_BINDIR})

    if (BUILD_OPENCV)
        ADD_EXECUTABLE(routio_camera src/apps/cameraserver.cpp)
        TARGET_LINK_LIBRARIES(routio_camera routio ${OpenCV_LIBS})
        ADD_EXECUTABLE(routio_image src/apps/imageserver.cpp)
        TARGET_LINK_LIBRARIES(routio_image routio ${OpenCV_LIBS})
        ADD_EXECUTABLE(routio_video src/apps/videoserver.cpp)
        TARGET_LINK_LIBRARIES(routio_video routio ${OpenCV_LIBS})
        ADD_EXECUTABLE(routio_client src/apps/videoclient.cpp)
        TARGET_LINK_LIBRARIES(routio_client routio ${OpenCV_LIBS})
        install(TARGETS routio_camera routio_image routio_video routio_client DESTINATION ${CMAKE_INSTALL_BINDIR})
        
    endif()
endif()

ADD_CUSTOM_TARGET(copy_library_python ALL)

SET(BUILD_PYTHON_DIR "${CMAKE_BINARY_DIR}/python")
FILE(MAKE_DIRECTORY ${BUILD_PYTHON_DIR}/routio)

SET(SOURCES_PY 
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/__main__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/ignition.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/tornado.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/_image.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/__main__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/library/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/library/geometry.msg
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/templates/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/templates/cpp.tpl
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/templates/python.tpl
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/python/routio/messages/cli.py
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/python/setup.py.in ${BUILD_PYTHON_DIR}/setup.py @ONLY)
ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/python/pyproject.toml ${BUILD_PYTHON_DIR}/)
ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/python/README.md ${BUILD_PYTHON_DIR}/)
ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/python/routio ${BUILD_PYTHON_DIR}/routio)


# Python bindings
IF(BUILD_PYTHON)

    find_path(PYBIND11_INCLUDE_DIR pybind11/pybind11.h HINTS /usr/include DOC "PyBind11 include directory")
    find_package(Python REQUIRED COMPONENTS Interpreter Development NumPy)

    add_library(pyroutio SHARED src/python/wrapper.cpp ${LIBRARY_SRC})
    set_target_properties(pyroutio PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${BUILD_PYTHON_DIR}/routio)
    set_target_properties(pyroutio PROPERTIES PREFIX "")
    target_include_directories(pyroutio PRIVATE ${Python_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIR})

    install(CODE "execute_process(COMMAND ${Python_EXECUTABLE} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)")

ELSE()

    FILE(MAKE_DIRECTORY ${BUILD_PYTHON_DIR}/routio/routio)

    ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND 
        ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} 
        ${CMAKE_COMMAND} -E copy ${LIBRARY_HEADERS} ${LIBRARY_SRC} src/python/wrapper.cpp src/algorithms.h src/debug.h
        ${BUILD_PYTHON_DIR}/routio/routio)
    ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ${CMAKE_COMMAND} -E copy ${LIBRARY_HEADERS} ${BUILD_PYTHON_DIR}/routio/routio)

    # Create dummy __init__.py to make routio a package
    FILE(WRITE ${BUILD_PYTHON_DIR}/routio/routio/__init__.py "# Dummy init file\n")

ENDIF()

# Examples
if(BUILD_EXAMPLES)
    add_executable(chat src/examples/chat.cpp)
    target_link_libraries(chat routio)

    add_executable(chunked src/examples/chunked.cpp)
    target_link_libraries(chunked routio)
endif()

# Examples
if(BUILD_TESTS)
    add_executable(deadend src/tests/deadend.cpp)
    target_link_libraries(deadend routio)

    add_executable(dynamic_client src/tests/dynamic_client.cpp)
    target_link_libraries(dynamic_client routio)

    add_executable(test_tensor src/tests/tensor.cpp)
    target_link_libraries(test_tensor routio)

endif()
