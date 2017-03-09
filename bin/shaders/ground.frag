#version 330

in vec4 texCoord;

out vec4 outColor;
uniform sampler2D groundTexture;

void main() {
	vec2 texCoord_2D;
	texCoord_2D.x = texCoord.x;
	texCoord_2D.y = texCoord.y;
    outColor = 0.3 * texture(groundTexture, texCoord_2D);
}
