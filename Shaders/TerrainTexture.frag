#version 440 core

#define MAX_TEXTURE_UNITS 16

in vec2 vUV;

out vec4 FragColor;

struct LoadedTexture {
    sampler2D texture;
    float height;
    vec2 tiling;
    vec2 offset;
};

uniform int textureCount;
uniform LoadedTexture loadedTextures[MAX_TEXTURE_UNITS];
uniform bool coloringMode;
uniform sampler2D heightMap;

vec4 CalcTexColor() {
    vec4 TexColor = vec4(1.0);

    if(textureCount == 0) {
        return TexColor;
    }

    float Height = texture(heightMap, vUV).r;
    if(Height < loadedTextures[0].height) {
        vec2 texCoord = vUV * loadedTextures[0].tiling + loadedTextures[0].offset;
        TexColor = texture(loadedTextures[0].texture, texCoord);
    } else if(Height > loadedTextures[textureCount - 1].height) {
        vec2 texCoord = vUV * loadedTextures[textureCount - 1].tiling + loadedTextures[textureCount - 1].offset;
        TexColor = texture(loadedTextures[textureCount - 1].texture, texCoord);
    } else {
        for(int i = 0; i < textureCount - 1; i++) {
            if(Height >= loadedTextures[i].height && Height <= loadedTextures[i + 1].height) {
                vec2 texCoord0 = vUV * loadedTextures[i].tiling + loadedTextures[i].offset;
                vec2 texCoord1 = vUV * loadedTextures[i + 1].tiling + loadedTextures[i + 1].offset;
                vec4 Color0 = texture(loadedTextures[i].texture, texCoord0);
                vec4 Color1 = texture(loadedTextures[i + 1].texture, texCoord1);
                float Delta = loadedTextures[i + 1].height - loadedTextures[i].height;
                float Factor = (Height - loadedTextures[i].height) / Delta;
                TexColor = mix(Color0, Color1, Factor);
                break;
            }
        }
    }

    return TexColor;
}

void main() {
    vec4 finalColor = vec4(1.0);
    if(coloringMode) {
        finalColor = CalcTexColor();
    }
    FragColor = finalColor;

    // debug draw vUV
    // FragColor = vec4(vUV, 0.0, 1.0);
}