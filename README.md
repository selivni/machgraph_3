# Mash3
3rd task on computer graphics

This program draws grass in 3-dimentional space and simulates the physical impact of the wind on it (wind is toggled by 'w' or 'r')
buttons

To compile everything run in the project directory:

> make

__To run the project, go to bin and run:__

> ./mash3 

## Possible errors:

If you get an error:
```
Shader: 0:1(10): error: GLSL 3.30 is not supported. Supported versions are: 1.10, 1.20, 1.30, 1.00 ES, and 3.00 ES

COMPILATION ERROR | Line: 22 File: shaders/grass.vert
```
Then you sould add this line to the end of ".bashrc" file in your home directory:
```
export MESA_GL_VERSION_OVERRIDE="3.3COMPAT"
```

If you your system does not support GLSL 3.10 - replace "shaders" directory and "main.cpp" with the ones from downgrade (which doesn't exist anymore)

For more information about the task, check .pdf file (in Russian)
