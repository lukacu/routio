cmake_minimum_required(VERSION 3.11)

project(routio CXX)
set(PROJECT_VERSION 0.1.0)

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_APPS "Build routio apps" ON)
option(BUILD_PYTHON "Build python wrapper" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DEBUG "Enable debug output" OFF)
option(BUILD_TESTS "Build tests" OFF)

find_package(OpenCV QUIET COMPONENTS core videoio highgui)

if (OpenCV_FOUND)
    set(BUILD_OPENCV ON)
    include_directories(${OpenCV_INCLUDE_DIR})
endif()

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif ()
else ()
    set (CMAKE_CXX_STANDARD 17)
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

set(ECHO_SRC 
    src/array.cpp
    src/loop.cpp
    src/message.cpp
    src/client.cpp
    src/server.cpp
    src/routing.cpp
    src/datatypes.cpp
    src/debug.cpp
)

set(ECHO_HEADERS 
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

# echo shared library
add_library(routio SHARED ${ECHO_SRC})

target_compile_options(routio PUBLIC "-pthread")
target_link_libraries(routio PUBLIC "pthread")

if (BUILD_OPENCV)
target_compile_definitions(routio PUBLIC "BUILD_OPENCV")
TARGET_LINK_LIBRARIES(routio PUBLIC ${OpenCV_LIBS})
endif()

#target_compile_options(echo_static PUBLIC "-pthread" "-fPIC")
#target_link_libraries(echo_static PUBLIC "pthread")

set_target_properties(routio PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)

install(TARGETS routio EXPORT routio_targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${ECHO_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/routio)

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

# echo daemon
if(BUILD_APPS)
    add_executable(routio-router src/apps/router.cpp)
    target_link_libraries(routio-router routio)
    install(TARGETS routio-router DESTINATION ${CMAKE_INSTALL_BINDIR})

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

# Python bindings
if(BUILD_PYTHON)
    option(BUILD_PYTHON_SOURCE "Build python source package" OFF)

    ADD_CUSTOM_TARGET(copy_library_python ALL)

    SET(BUILD_PYTHON_DIR "${CMAKE_BINARY_DIR}/python")
    FILE(MAKE_DIRECTORY ${BUILD_PYTHON_DIR}/routio)

    IF(BUILD_PYTHON_SOURCE)

        FILE(MAKE_DIRECTORY ${BUILD_PYTHON_DIR}/routio/routio)

        ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND 
            ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} 
            ${CMAKE_COMMAND} -E copy ${ECHO_SRC} src/python/wrapper.cpp src/algorithms.h src/debug.h
            ${BUILD_PYTHON_DIR}/routio/routio)
        ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} ${CMAKE_COMMAND} -E copy ${ECHO_HEADERS} ${BUILD_PYTHON_DIR}/routio/routio)

    ELSE()

        find_path(PYBIND11_INCLUDE_DIR pybind11/pybind11.h HINTS /usr/include DOC "PyBind11 include directory")
        find_package(Python REQUIRED COMPONENTS Interpreter Development NumPy)

        add_library(pyroutio SHARED src/python/wrapper.cpp ${ECHO_SRC})
        set_target_properties(pyroutio PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${BUILD_PYTHON_DIR}/routio)
        set_target_properties(pyroutio PROPERTIES PREFIX "")
        target_include_directories(pyroutio PRIVATE ${Python_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIR})

        install(CODE "execute_process(COMMAND ${Python_EXECUTABLE} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)")

    ENDIF()


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


    ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/python/setup.py ${BUILD_PYTHON_DIR}/)
    ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/python/pyproject.toml ${BUILD_PYTHON_DIR}/)
    ADD_CUSTOM_COMMAND(TARGET copy_library_python COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/python/routio ${BUILD_PYTHON_DIR}/routio)


endif()

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
