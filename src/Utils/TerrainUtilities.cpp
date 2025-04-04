#include "../Core/TerrainData.h"

namespace TerrainUtilities {
    float GenerateFalloffValue(float x, float z, const FalloffParameters& falloffParams) {
        if (!falloffParams.enabled)
            return 1.0f;

        // Normalize
        float normalizedX = (x + 1.0f) * 0.5f;
        float normalizedZ = (z + 1.0f) * 0.5f;
        normalizedX -= 0.5f;
        normalizedZ -= 0.5f;

        float distanceFromCenter;
        switch (falloffParams.type) {
        case TerrainUtilities::FalloffType::CIRCULAR:
            distanceFromCenter = 2.0f * glm::sqrt(glm::pow(normalizedX, 2) + glm::pow(normalizedZ, 2));
            break;
        case TerrainUtilities::FalloffType::DIAMOND:
            distanceFromCenter = 2.0f * (glm::abs(normalizedX) + glm::abs(normalizedZ));
            break;
        case TerrainUtilities::FalloffType::SQUARE:
        default:
            // Square falloff (max of x and z distance)
            distanceFromCenter = 2.0f * glm::max(glm::abs(normalizedX), glm::abs(normalizedZ));
            break;
        }

        // Ensure we're within bounds
        distanceFromCenter = glm::clamp(distanceFromCenter, 0.0f, 1.0f);

        // Apply the falloff curve: f(x) = x^a / (x^a + (b-bx)^a)
        float value = glm::pow(distanceFromCenter, falloffParams.a) /
            (glm::pow(distanceFromCenter, falloffParams.a) + glm::pow(falloffParams.b - falloffParams.b * distanceFromCenter, falloffParams.a));

        // Invert
        return 1.0f - value;
    }
}