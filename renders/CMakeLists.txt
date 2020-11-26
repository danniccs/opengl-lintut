target_sources(gltut
    PRIVATE
        quick_normals.cpp
)

target_include_directories(gltut
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
