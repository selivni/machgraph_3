#version 330

in vec4 point;
in vec2 position;
in vec4 variance;
in vec2 difference;
out vec2 diff;
out vec4 point_out;

in vec4 texCoord_in;
out vec4 texCoord;

uniform mat4 camera;



void main() {
	float pi = 3.1415;
	diff = difference;
	texCoord = texCoord_in;
	float atanab;
	float h;
	point_out = point;
    mat4 scaleMatrix = mat4(1.0);
	mat4 rotationMatrix = mat4(0.0);
	vec4 var = variance;
	rotationMatrix[0][0] = cos(difference.y*pi);
	rotationMatrix[0][2] = -sin(difference.y*pi);
	rotationMatrix[2][0] = sin(difference.y*pi);
	rotationMatrix[1][1] = 1;
	rotationMatrix[3][3] = 1;
	rotationMatrix[2][2] = cos(difference.y*pi);

    scaleMatrix[0][0] = 0.01*difference.x;
    scaleMatrix[1][1] = 0.1*difference.x;
    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x-0.5;
	positionMatrix[3][1] = positionMatrix[3][1]-1;
    positionMatrix[3][2] = position.y-0.5;
	positionMatrix = positionMatrix * rotationMatrix;
	var = variance * 16;
	atanab = atan(var.x);
	var.x = sin(atanab);
	var.y = -1+cos(atanab);
	gl_Position = camera * (positionMatrix * scaleMatrix * point + var * (sqrt(point.y * point.y * point.y) * scaleMatrix[1][1]));
}
