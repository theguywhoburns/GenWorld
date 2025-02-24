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
        float lacunarity = 2.0f;	// Adjust for frequency of noise
        float persistence = 0.5f;	// Adjust for amplitude of noise
        float scale = 2.0f;			// Adjust for overall height of terrain

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