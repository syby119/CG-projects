## CG Projects

#### Introduction
This is the assignments for the Computer Graphics course, through which you'll learn how to:
+ construct the simplest framework for a Computer Graphics program;
+ display geometry and pixel data on the screen;
+ use modern OpenGL API (3.3+);
+ use GLSL, a Shading Language compatible with OpenGL;

This will help you understanding:
+ the basic concepts in Computer Graphics: 
    + geometry
    + lighting
    + texture
    + graphics pipleine etc;
+ how GPU works to display a frame on the screen;

Basically, I follow the [Learning OpenGL](https://learnopengl-cn.github.io/) tutorials and refactor the old version assignments in an object oriented way. The students should have some knowledge of modern C++. Don't be panic if you haven't learnt C++ yet. You'll write no more than 20 lines of code for most of the assignments.

You can find more materials in [Bilibili](https://space.bilibili.com/52683403/channel/collectiondetail?sid=749547&ctype=0)

#### How to run

##### Preliminaries
+ install cmake >= 3.20
+ C++ Compiler supports at least C++14

##### Build and Compile
```shell
cmake -Bbuild .
cd build
cmake --build . --parallel 8
```

##### Run
```shell
cd bin/Debug
./project1.exe
```

#### Windows Executables
##### Render a Triangle
<div style="text-align:center">
    <img src="./screenshots/get_start.png" width="640"/>
</div>

##### Render the Chinese National Flag
<div style="text-align:center">
    <img src="./screenshots/project1.png" width="640"/>
</div>

##### Transformation Matrices
<div style="text-align:center">
    <img src="./screenshots/project2.png" width="640"/>
</div>

##### Scene Roaming
<div style="text-align:center">
    <img src="./screenshots/project3.png" width="640"/>
</div>

##### Instanced Rendering
<div style="text-align:center">
    <img src="./screenshots/project4.png" width="640"/>
</div>

##### Shading Tutorial
<div style="text-align:center">
    <img src="./screenshots/project5.png" width="640"/>
</div>

##### Texture Mapping
<div style="text-align:center">
    <img src="./screenshots/project6.png" width="640"/>
</div>

##### Transparency Rendering
<div style="text-align:center">
    <img src="./screenshots/bonus1.png" width="640"/>
</div>

##### Frustum Culling
<div style="text-align:center">
    <img src="./screenshots/bonus2.png" width="640"/>
</div>

##### Post Processing
<div style="text-align:center">
    <img src="./screenshots/bonus3.png" width="640"/>
</div>

##### Shadow Mapping
<div style="text-align:center">
    <img src="./screenshots/bonus4.png" width="640"/>
</div>

##### Ray Tracing
<div style="text-align:center">
    <img src="./screenshots/bonus5.png" width="640"/>
</div>

##### PBR Viewer
<div style="text-align:center">
    <img src="./screenshots/pbr_viewer.png" width="640"/>
</div>