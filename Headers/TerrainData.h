#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include <Drawables/Mesh.h>
#include <ImGuiCurveTest.h>

namespace TerrainUtilities
{
    struct VertexColor {
        float height;
        glm::vec4 color;
    };

    struct TerrainData
    {
        // Terrain Data
        float width;
        float length;
        int cellSize;
        float heightMultiplier;
        unsigned int numCellsWidth;
        unsigned int numCellsLength;
        float stepX;
        float stepZ;
        ImGui::point* curvePoints;

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
    };
}