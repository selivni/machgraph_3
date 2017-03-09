#version 330 core

in vec4 position;
out vec4 TexCoord;

uniform mat4 camera;

void main()
{
	mat4 scaleMatrix = mat4(1.0);
	scaleMatrix[3][3] = 0.1;
	gl_Position = camera * scaleMatrix * position;
	TexCoord = position;
}
