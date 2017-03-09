#version 330

in vec4 point;
in vec4 texCoord_in;

out vec4 texCoord;

uniform mat4 camera;

void main() {
	vec4 temp = point;
	temp.x = temp.x-0.5;
	temp.y = temp.y-1;
	temp.z = temp.z-0.5;
	mat4 ScaleMatrix = mat4(1.0);
	ScaleMatrix[0][0] = 3;
	ScaleMatrix[2][2] = 3;
	texCoord = texCoord_in;
    gl_Position = camera * ScaleMatrix * temp;
}
