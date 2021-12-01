cmake_minimum_required(VERSION 3.13)

project(OpenGLTutorial VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

add_library(srclib "")
add_executable(gltut "")

add_subdirectory(src)
add_subdirectory(renders)

find_package(glfw3 3.3 REQUIRED)

find_package(assimp REQUIRED)
if (assimp_FOUND)
    set(ASSIMP_LIBRARY "assimp")
    add_library(${ASSIMP_LIBRARY} SHARED IMPORTED)
    set_target_properties(${ASSIMP_LIBRARY} PROPERTIES
        IMPORTED_LOCATION "${ASSIMP_LIBRARY_DIRS}/libassimp.so"
    )
endif(assimp_FOUND)

target_link_libraries(gltut glfw srclib ${ASSIMP_LIBRARY})

target_link_options(gltut
    PUBLIC
        -lglfw3
        -lGL
        -lX11
        -lpthread
        -lXrandr
        -lXi
        -ldl
)

target_compile_options(gltut
    PUBLIC
        -pedantic
        -Wall
        -Wextra
        -Wno-unused-parameter
        -O3
)

#install(TARGETS OpenGLTutorial DESTINATION ${PROJECT_BINARY_DIR})
