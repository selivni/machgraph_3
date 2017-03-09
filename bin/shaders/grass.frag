#version 330

in vec2 diff;

in vec4 texCoord;
in vec4 point_out;

uniform sampler2D grassTexture;

out vec4 outColor;

void main() {
	vec2 texCoord2D;
	texCoord2D.x = texCoord.x;
	texCoord2D.y = texCoord.y;
	outColor = 0.3*(point_out.y*0.7+0.4)*texture(grassTexture,texCoord2D);
}
