# opengl-lintut
Implementation of simple shadow mapping using OpenGL.

### Requirements
Requires:
- [GLFW 3.3](https://www.glfw.org/)
- [Assimp](http://assimp.org/)
- [GLAD](https://github.com/Dav1dde/glad) (You can download a generated file [here](https://glad.dav1d.de/))

GLM is also used, but is included in the repository.

To obtain the textures used, please visit [freepbr.com](https://freepbr.com/).
The textures are in the "Metals" category:
- Streaky Metal 1
- Rusted Iron Alt 2
- Worn Metal 4

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
