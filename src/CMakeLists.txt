target_sources(srclib
    PRIVATE
        glad.c
        Light.cpp
        Mesh.cpp
        misc_sources.cpp
        Model.cpp
        Shader.cpp
        SimpleMesh.cpp
        stb_img_implementation.cpp
        texture_loader.cpp
)

target_include_directories(srclib
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
