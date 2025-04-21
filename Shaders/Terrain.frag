#version 440 core

#define MAX_TEXTURE_UNITS 16

in vec3 vertexNormal;
in vec3 vertexColor;
in vec2 vertexTexCoord;

out vec4 FragColor;

uniform int textureCount;
uniform sampler2D textureSampler[MAX_TEXTURE_UNITS];

void main() {
	vec4 finalColor = vec4(vertexColor, 1.0);

	for(int i = 0; i < textureCount; i++) {
		vec4 texColor = texture(textureSampler[i], vertexTexCoord);
		finalColor *= texColor;
	}

	FragColor = finalColor;
}
