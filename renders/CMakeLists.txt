target_sources(gltut
    PRIVATE
        shadows.cpp
)

target_include_directories(gltut
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
