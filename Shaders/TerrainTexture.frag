#version 440 core

#define MAX_TEXTURE_UNITS 16
#define MAX_COLOR_UNITS 32

in vec2 vertexTexCoord;

out vec4 FragColor;

struct LoadedTexture {
    sampler2D texture;
    float height;
    vec2 tiling;
    vec2 offset;
};

struct ColorData {
    vec4 color;
    float height;
};

uniform int textureCount;
uniform LoadedTexture loadedTextures[MAX_TEXTURE_UNITS];
uniform int colorCount;
uniform ColorData colors[MAX_COLOR_UNITS];
uniform bool coloringMode;
uniform sampler2D heightmap;

vec4 CalcTexColor() {
    vec4 TexColor = vec4(1.0);

    if(textureCount == 0) {
        return TexColor;
    }

    float Height = texture(heightmap, vertexTexCoord).r;

    if(textureCount == 1) {
        vec2 texCoord = vertexTexCoord * loadedTextures[0].tiling + loadedTextures[0].offset;
        TexColor = texture(loadedTextures[0].texture, texCoord);
        return TexColor;
    }

    if(Height < loadedTextures[0].height) {
        vec2 texCoord = vertexTexCoord * loadedTextures[0].tiling + loadedTextures[0].offset;
        TexColor = texture(loadedTextures[0].texture, texCoord);
    } else if(Height > loadedTextures[textureCount - 1].height) {
        vec2 texCoord = vertexTexCoord * loadedTextures[textureCount - 1].tiling + loadedTextures[textureCount - 1].offset;
        TexColor = texture(loadedTextures[textureCount - 1].texture, texCoord);
    } else {
        for(int i = 0; i < textureCount - 1; i++) {
            if(Height >= loadedTextures[i].height && Height <= loadedTextures[i + 1].height) {
                vec2 texCoord0 = vertexTexCoord * loadedTextures[i].tiling + loadedTextures[i].offset;
                vec2 texCoord1 = vertexTexCoord * loadedTextures[i + 1].tiling + loadedTextures[i + 1].offset;
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

vec4 CalcColor() {
    if(colorCount == 0) {
        return vec4(1.0);
    }

    float Height = texture(heightmap, vertexTexCoord).r;
    if(Height <= colors[0].height)
        return colors[0].color;

    for(int i = 0; i < colorCount - 1; i++) {
        if(Height <= colors[i].height) {
            return colors[i].color;
        }
    }
    
    return colors[colorCount - 1].color;
}

void main() {
    vec4 finalColor = vec4(1.0);
    if(coloringMode) {
        finalColor = CalcTexColor();
    } else {
        finalColor = CalcColor();
    }
    FragColor = finalColor;
}
