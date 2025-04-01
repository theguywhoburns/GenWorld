#include "TerrainUI.h"

TerrainUI::TerrainUI(TerrainController* controller) : controller(controller) {
    // Terrain Data
    parameters.width = 100;
    parameters.length = 100;
    parameters.cellSize = 1;
    parameters.heightMultiplier = 7;
    parameters.curvePoints = new ImGui::point[4] { {0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.5f} };

    // Noise Data
    parameters.lacunarity = 2.0f;
    parameters.persistence = 0.5f;
    parameters.scale = 50.0f;
    parameters.octaves = 4;
    parameters.seed = 0;
    parameters.offset = glm::vec2(0.0f, 0.0f);

    parameters.colors = {
        {0.1f, glm::vec4(0.15f, 0.22f, 0.34f, 1.0f)},   // Deep Water (Dark Blue)
        {0.15f, glm::vec4(0.2f, 0.4f, 0.6f, 1.0f)},     // Shallow Water (Lighter Blue)
        {0.25f, glm::vec4(0.8f, 0.7f, 0.4f, 1.0f)},     // Sand (Yellowish)
        {0.35f, glm::vec4(0.3f, 0.6f, 0.2f, 1.0f)},     // Grass (Green)
        {0.5f, glm::vec4(0.4f, 0.7f, 0.3f, 1.0f)},      // Lush Grass (Brighter Green)
        {0.65f, glm::vec4(0.5f, 0.4f, 0.2f, 1.0f)},     // Dirt (Brown)
        {0.9f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)},      // Rock (Gray)
        {1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},      // Snow (White)
    };
}

void TerrainUI::DisplayUI() {
    ImGui::Begin("Terrain Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Text("Terrain Settings");
    ImGui::DragFloat("Width", &parameters.width, 0.1f, 1, 100);
    ImGui::DragFloat("Length", &parameters.length, 0.1f, 1, 100);
    ImGui::SliderInt("Division Size", &parameters.cellSize, 1, 10);
    ImGui::DragFloat("Height Multiplier", &parameters.heightMultiplier, 0.1f, 1, 100);
    ImGui::DrawCurve("easeOutSine", parameters.curvePoints);

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::Text("Noise Settings");
    ImGui::SliderFloat("Scale", &parameters.scale, 0.001f, 50.0f);
    ImGui::SliderInt("Octaves", &parameters.octaves, 1, 10);
    ImGui::SliderFloat("Lacunarity", &parameters.lacunarity, 0.1f, 50.0f);
    ImGui::SliderFloat("Persistence", &parameters.persistence, 0.0f, 1.0f);
    ImGui::DragFloat2("Offset", &parameters.offset[0], 0.1f);
    ImGui::DragInt("Seed", &parameters.seed, 1, 0, 10000);

    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::Button("Randomize Seed", ImVec2(200, 40))) {
        parameters.seed = rand() % 10000;
    }

    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        controller->Generate();
    }

    ImGui::End();

    ImGui::Begin("Color Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Text("Color Settings");

    for (size_t i = 0; i < parameters.colors.size(); i++) {
        std::string index = std::to_string(i + 1);
        ImGui::Text(("Color " + index).c_str());

        ImGui::ColorEdit3(("Color##" + index).c_str(), &parameters.colors[i].color[0]);
        ImGui::SliderFloat(("Height##" + index).c_str(), &parameters.colors[i].height, 0.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));         // Red background
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Lighter red on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));   // Dark red when clicked
        ImGui::SameLine();
        if (ImGui::Button(("X##" + index).c_str())) {
            parameters.colors.erase(parameters.colors.begin() + i);
            ImGui::PopStyleColor(3);
            continue;
        }
        ImGui::PopStyleColor(3);

        if (i < parameters.colors.size() - 1) {
            ImGui::SameLine();
            if (ImGui::ArrowButton(("Down##" + index).c_str(), ImGuiDir_Down)) {
                std::swap(parameters.colors[i], parameters.colors[i + 1]);
            }
        }
        if (i > 0) {
            ImGui::SameLine();
            if (ImGui::ArrowButton(("Up##" + index).c_str(), ImGuiDir_Up)) {
                std::swap(parameters.colors[i], parameters.colors[i - 1]);
            }
        }
    }

    if (ImGui::Button("Add Color")) {
        TerrainUtilities::VertexColor defaultColor;
        defaultColor.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        defaultColor.height = 0.5f;
        parameters.colors.push_back(defaultColor);
    }

    ImGui::End();
}

TerrainUtilities::TerrainData TerrainUI::GetParameters() const {
    return parameters;
}
