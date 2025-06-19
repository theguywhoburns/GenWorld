#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <memory>

#include "../Drawables/Mesh.h"
#include "ImGuiCurveTest.h"


namespace TerrainUtilities {
    struct VertexColor {
        float height;
        glm::vec4 color;
    };

    struct TextureData {
        std::shared_ptr<Texture> texture;
        float height;
        glm::vec2 tiling;
        glm::vec2 offset;
    };

    // Falloff
    enum FalloffType {
        SQUARE,
        CIRCULAR,
        DIAMOND
    };

    struct FalloffParameters {
        bool enabled = true;
        float a = 3.0f;
        float b = 2.5f;
        FalloffType type = SQUARE;
    };

    // Decoration Rules
    struct DecorationRule {
        glm::vec2 heightLimits;
        glm::vec2 scaleRange;
        bool randomRotation;
        float density;
        std::string modelPath;
    };

    struct TerrainData {
        // Terrain Data
        float width;
        float length;
        float halfWidth;
        float halfLength;
        int cellSize;
        unsigned int numCellsWidth;
        unsigned int numCellsLength;
        float stepX;
        float stepZ;
        float maxHeight;

        // Height Multiplier
        float heightMultiplier;
        vector<ImGui::point> curvePoints;

        // Noise Parameters
        float lacunarity;
        float persistence;
        float scale;

        // Noise Settings
        int octaves;
        int seed;
        glm::vec2 offset;
        vector<glm::vec2> octaveOffsets;

        // Color Data
        vector<VertexColor> colors;

        // Texture Data
        vector<TextureData> loadedTextures;

        int coloringMode = 0; // 0: Color, 1: Texture

        // Falloff Data
        FalloffParameters falloffParams;

        // Decoration Rules
        bool decorationEnabled = false;
        vector<DecorationRule> decorationRules;

        bool operator==(const TerrainData& other) const {
            for (int i = 0; i < 4; ++i) {
                if (curvePoints[i].x != other.curvePoints[i].x || curvePoints[i].y != other.curvePoints[i].y) {
                    return false;
                }
            }

            if (colors.size() != other.colors.size()) {
                return false;
            }

            for (size_t i = 0; i < colors.size(); ++i) {
                if (colors[i].height != other.colors[i].height || colors[i].color != other.colors[i].color) {
                    return false;
                }
            }

            if (loadedTextures.size() != other.loadedTextures.size()) {
                return false;
            }

            for (size_t i = 0; i < loadedTextures.size(); ++i) {
                if (loadedTextures[i].texture.get()->path != other.loadedTextures[i].texture.get()->path) {
                    return false;
                }
                if (loadedTextures[i].height != other.loadedTextures[i].height) {
                    return false;
                }
                if (loadedTextures[i].tiling != other.loadedTextures[i].tiling || loadedTextures[i].offset != other.loadedTextures[i].offset) {
                    return false;
                }
            }

            if (decorationRules.size() != other.decorationRules.size()) {
                return false;
            }

            for (size_t i = 0; i < decorationRules.size(); ++i) {
                if (decorationRules[i].heightLimits != other.decorationRules[i].heightLimits ||
                    decorationRules[i].scaleRange != other.decorationRules[i].scaleRange ||
                    decorationRules[i].randomRotation != other.decorationRules[i].randomRotation ||
                    decorationRules[i].density != other.decorationRules[i].density ||
                    decorationRules[i].modelPath != other.decorationRules[i].modelPath) {
                    return false;
                }
            }

            return width == other.width &&
                length == other.length &&
                cellSize == other.cellSize &&
                heightMultiplier == other.heightMultiplier &&
                lacunarity == other.lacunarity &&
                persistence == other.persistence &&
                scale == other.scale &&
                octaves == other.octaves &&
                seed == other.seed &&
                offset == other.offset &&
                coloringMode == other.coloringMode &&
                falloffParams.enabled == other.falloffParams.enabled &&
                falloffParams.a == other.falloffParams.a &&
                falloffParams.b == other.falloffParams.b &&
                falloffParams.type == other.falloffParams.type &&
                decorationEnabled == other.decorationEnabled;
        }

        bool operator!=(const TerrainData& other) const {
            return !(*this == other);
        }
    };

    float GenerateFalloffValue(float x, float z, const FalloffParameters& falloffParams);
}