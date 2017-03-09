#version 330 core

in vec4 TexCoord;
out vec4 color;

uniform samplerCube skybox;

void main()
{
	vec3 TexCoord3D;
	TexCoord3D.x = TexCoord.x;
	TexCoord3D.y = TexCoord.y;
	TexCoord3D.z = TexCoord.z;
	color = texture(skybox, TexCoord3D);
}
