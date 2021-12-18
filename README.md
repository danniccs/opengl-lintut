# opengl-lintut
Implementation of shadow mapping and PBR with area lights using OpenGL.

### Requirements
Requires:
- [GLFW 3.3](https://www.glfw.org/)
- [Assimp](http://assimp.org/)
- [GLAD](https://github.com/Dav1dde/glad) (You can download a generated file [here](https://glad.dav1d.de/))

GLM is also used, but is included in the repository.

To obtain the textures and model used, please visit [freepbr.com](https://freepbr.com/).
The texture and model folders should be placed inside resources/pbr_textures.
The model used was the Blender version of Sharp Boulder 2.

The Unreal Engine versions of the following textures were used:
- Streaky Metal 1
- Rusted Iron Alt 2
- Worn Metal 4
- Gray Granite Flecks 1
- Rich Brown Tile Variation

### Getting Started
To compile and run:
```console
foo@bar:~/openg-lintut$ mkdir build
foo@bar:~/openg-lintut$ cd build
foo@bar:~/openg-lintut/build$ cmake ..
foo@bar:~/openg-lintut/build$ make
foo@bar:~/openg-lintut/build$ cd ..
foo@bar:~/openg-lintut/build$ ./gltut
```
### Controls
Use WASD to move around, move up with Space and down with C.
To switch between a tube light and sphere light, press T. When rendering with a sphere light, press P to toggle between
a point light and area light approximation.
Press L to toggle wireframe rendering on/off.