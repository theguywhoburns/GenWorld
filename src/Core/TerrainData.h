#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include "../Drawables/Mesh.h"
#include "ImGuiCurveTest.h"


namespace TerrainUtilities {
    struct VertexColor {
        float height;
        glm::vec4 color;
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

    struct TerrainData {
        // Terrain Data
        float width;
        float length;
        int cellSize;
        float heightMultiplier;
        unsigned int numCellsWidth;
        unsigned int numCellsLength;
        float stepX;
        float stepZ;
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
        float halfWidth;
        float halfLength;

        // Falloff Data
        FalloffParameters falloffParams;

        bool operator==(const TerrainData& other) const {
            for(int i = 0; i < 4; ++i) {
                if (curvePoints[i].x != other.curvePoints[i].x || curvePoints[i].y != other.curvePoints[i].y) {
                    return false;
                }
            }

            if(colors.size() != other.colors.size()) {
                return false;
            }

            for (size_t i = 0; i < colors.size(); ++i) {
                if (colors[i].height != other.colors[i].height || colors[i].color != other.colors[i].color) {
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
                falloffParams.enabled == other.falloffParams.enabled &&
                falloffParams.a == other.falloffParams.a &&
                falloffParams.b == other.falloffParams.b &&
                falloffParams.type == other.falloffParams.type;
        }

        bool operator!=(const TerrainData& other) const {
            return !(*this == other);
        }
    };

    float GenerateFalloffValue(float x, float z, const FalloffParameters& falloffParams);
}