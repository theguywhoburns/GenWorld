#include "TerrainUI.h"

TerrainUI::TerrainUI(TerrainController *controller) : controller(controller)
{

    // Terrain Data
    parameters.width = 100;
    parameters.length = 100;
    parameters.cellSize = 1;
    parameters.heightMultiplier = 35;
    parameters.curvePoints = {{0.0f, 0.0f}, {1.0f, 1.0f}};

    // Noise Data
    parameters.lacunarity = 2.0f;
    parameters.persistence = 0.5f;
    parameters.scale = 50.0f;
    parameters.octaves = 4;
    parameters.seed = 2258;
    parameters.offset = glm::vec2(0.0f, 0.0f);

    parameters.loadedTextures = {
        {std::make_shared<Texture>("Textures/Water_001_COLOR.jpg"), 0.0f, glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
        {std::make_shared<Texture>("Textures/coast_sand_01_diff_1k.jpg"), 0.1f, glm::vec2(5.0f, 5.0f), glm::vec2(0.0f, 0.0f)},
        {std::make_shared<Texture>("Textures/brown_mud_leaves_01_diff_1k.jpg"), 0.25f, glm::vec2(5.0f, 5.0f), glm::vec2(0.0f, 0.0f)},
        {std::make_shared<Texture>("Textures/aerial_rocks_04_diff_1k.jpg"), 0.35f, glm::vec2(5.0f, 5.0f), glm::vec2(0.0f, 0.0f)},
        {std::make_shared<Texture>("Textures/aerial_rocks_04_diff_1k.jpg"), 0.85f, glm::vec2(5.0f, 5.0f), glm::vec2(0.0f, 0.0f)},
        {std::make_shared<Texture>("Textures/snow_02_diff_1k.jpg"), 1.0f, glm::vec2(5.0f, 5.0f), glm::vec2(0.0f, 0.0f)},
    };

    parameters.colors = {
        {0.075f, glm::vec4(0.21f, 0.4f, 0.68f, 1.0f)},  // Shallow Water (Lighter Blue)
        {0.15f, glm::vec4(0.82f, 0.835f, 0.45f, 1.0f)}, // Sand (Yellowish)
        {0.35f, glm::vec4(0.35f, 0.68f, 0.24f, 1.0f)},  // Lush Grass (Brighter Green)
        {0.65f, glm::vec4(0.3f, 0.55f, 0.17f, 1.0f)},   // Grass (Green)
        {0.85f, glm::vec4(0.4f, 0.38f, 0.34f, 1.0f)},   // Rock (Gray)
        {1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},      // Snow (White)
    };
}

void TerrainUI::DisplayUI()
{
    DisplaySceneViewOverlay();

    TerrainUtilities::TerrainData prevParameters = parameters;

    // Terrain Settings
    DisplayTerrainSettingsUI();

    if (parameters.coloringMode == 0)
    {
        // Color Settings
        DisplayColorSettingsUI();
    }
    else
    {
        // Texture Settings
        DisplayTextureLayerSettings();
    }

    // Decoration Settings
    if (parameters.decorationEnabled)
    {
        DisplayDecorationSettings();
    }

    if (parameters != prevParameters && liveUpdate)
    {
        controller->Generate();
    }
}

void TerrainUI::DisplayMainSettingsWindow() {
    ImGui::Begin("Terrain Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    if (ImGui::BeginTabBar("TerrainTabs")) {

        // Terrain Tab
        if (ImGui::BeginTabItem("Terrain")) {
            DisplayTerrainSettingsTab();
            ImGui::EndTabItem();
        }

        // Colors/Textures Tab
        if (ImGui::BeginTabItem("Appearance")) {
            DisplayAppearanceTab();
            ImGui::EndTabItem();
        }

        // Decoration Tab
        if (ImGui::BeginTabItem("Decoration")) {
            DisplayDecorationTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void TerrainUI::DisplayTerrainSettingsTab() {
    ImGui::Text("Terrain Settings");
    ImGui::DragFloat("Width", &parameters.width, 0.1f, 1, 100);
    ImGui::DragFloat("Length", &parameters.length, 0.1f, 1, 100);
    ImGui::SliderInt("Division Size", &parameters.cellSize, 1, 10);
    ImGui::DragFloat("Height Multiplier", &parameters.heightMultiplier, 0.1f, 1, 100);

    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("Curve Settings");
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

    RenderFalloffControls();

    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::Button("Randomize Seed", ImVec2(200, 40))) {
        parameters.seed = rand() % 10000;
    }

    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        controller->Generate();
    }
}

void TerrainUI::DisplayAppearanceTab() {
    ImGui::Text("Coloring Mode");
    ImGui::RadioButton("Use Colors", &parameters.coloringMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Use Textures", &parameters.coloringMode, 1);

    ImGui::Separator();
    ImGui::NewLine();

    if (parameters.coloringMode == 0) {
        DisplayColorSettings();
    }
    else {
        DisplayTextureLayerSettings();
    }
}

void TerrainUI::DisplayDecorationTab() {
    ImGui::Checkbox("Enable Decoration", &parameters.decorationEnabled);

    ImGui::Separator();
    ImGui::NewLine();

    if (!parameters.decorationEnabled) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Decoration is disabled. Enable the checkbox above.");
        return;
    }

    DisplayDecorationSettings();
}

void TerrainUI::RenderFalloffControls() {
    ImGui::Checkbox("Enable Falloff", &parameters.falloffParams.enabled);
    if (parameters.falloffParams.enabled)
    {
        const char *types[] = {"Square", "Circular", "Diamond"};
        int currentType = (int)parameters.falloffParams.type;
        if (ImGui::Combo("Falloff Shape", &currentType, types, IM_ARRAYSIZE(types)))
        {
            parameters.falloffParams.type = (TerrainUtilities::FalloffType)currentType;
        }

        ImGui::SliderFloat("Steepness (a)", &parameters.falloffParams.a, 0.1f, 10.0f);
        ImGui::SliderFloat("Midpoint (b)", &parameters.falloffParams.b, 0.1f, 10.0f);

        // Preview window
        ImVec2 previewSize(200, 200);
        ImGui::BeginChild("FalloffPreview", previewSize, true);

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();

        const int resolution = 100;
        float cellWidth = previewSize.x / resolution;
        float cellHeight = previewSize.y / resolution;

        for (int y = 0; y < resolution; y++)
        {
            for (int x = 0; x < resolution; x++)
            {
                float normalizedX = (float)x / resolution;
                float normalizedZ = (float)y / resolution;

                float worldX = normalizedX * 2.0f - 1.0f;
                float worldZ = normalizedZ * 2.0f - 1.0f;

                float falloff = TerrainUtilities::GenerateFalloffValue(worldX, worldZ, parameters.falloffParams);

                unsigned char intensity = (unsigned char)(falloff * 255.0f);
                ImU32 color = IM_COL32(intensity, intensity, intensity, 255);

                drawList->AddRectFilled(
                    ImVec2(p.x + x * cellWidth, p.y + y * cellHeight),
                    ImVec2(p.x + (x + 1) * cellWidth, p.y + (y + 1) * cellHeight),
                    color);
            }
        }
        ImGui::EndChild();
    }
}

void TerrainUI::DisplaySceneViewOverlay()
{
    ImVec2 min_size(150.0f, 150.0f);
    ImVec2 max_size(INT16_MAX, INT16_MAX);
    ImGui::SetNextWindowSizeConstraints(min_size, max_size);

    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();

    const float padding = 10.0f;
    ImVec2 overlaySize(142, 50);
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
    ImVec2 overlayPos = ImVec2(contentRegionMax.x - overlaySize.x - padding, contentRegionMin.y + padding);

    ImGui::SetCursorScreenPos(windowPos + overlayPos);

    // Style overrides
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

    ImGui::BeginChild("SceneViewOverlay", overlaySize,
                      ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse |
                          ImGuiWindowFlags_NoTitleBar);
    
    // Force white text for the checkbox to ensure visibility on dark overlay
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Checkbox("Live Update", &liveUpdate);
    ImGui::PopStyleColor();
    // ImGui::Checkbox("Live Update", &liveUpdate);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::SetCursorScreenPos(originalCursorPos);
    ImGui::End();
}

void TerrainUI::DisplayTerrainSettingsUI()
{
    ImGui::Begin("Terrain Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::Spectrum::BeginTitleFont();
    ImGui::Text("Terrain Settings");
    ImGui::Spectrum::EndTitleFont();

    ImGui::Separator();
    ImGui::Spacing();

    // Regular controls use default font
    ImGui::DragFloat("Width", &parameters.width, 0.1f, 1, 100);
    ImGui::DragFloat("Length", &parameters.length, 0.1f, 1, 100);
    ImGui::SliderInt("Division Size", &parameters.cellSize, 1, 10);
    ImGui::DragFloat("Height Multiplier", &parameters.heightMultiplier, 0.1f, 1, 100);

    ImGui::Separator();
    ImGui::NewLine();

    // Section title with bold font
    ImGui::Spectrum::SectionTitle("Curve Settings");
    ImGui::DrawCurve("easeOutSine", parameters.curvePoints);

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    // Section title with bold font
    ImGui::Spectrum::SectionTitle("Noise Settings");
    ImGui::SliderFloat("Scale", &parameters.scale, 0.001f, 50.0f);
    ImGui::SliderInt("Octaves", &parameters.octaves, 1, 10);
    ImGui::SliderFloat("Lacunarity", &parameters.lacunarity, 0.1f, 50.0f);
    ImGui::SliderFloat("Persistence", &parameters.persistence, 0.0f, 1.0f);
    ImGui::DragFloat2("Offset", &parameters.offset[0], 0.1f);
    ImGui::DragInt("Seed", &parameters.seed, 1, 0, 10000);

    ImGui::Separator();
    ImGui::NewLine();

    ImGui::Checkbox("Enable Decoration", &parameters.decorationEnabled); // show decoration window

    ImGui::Separator();
    ImGui::NewLine();

    RenderFalloffControls();

    ImGui::Separator();
    ImGui::NewLine();

    ImGui::Spectrum::BeginTitleFont();
    ImGui::Text("Coloring Mode");
    ImGui::Spectrum::EndTitleFont();

    ImGui::RadioButton("Use Colors", &parameters.coloringMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Use Textures", &parameters.coloringMode, 1);

    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::Button("Randomize Seed", ImVec2(200, 40)))
    {
        parameters.seed = rand() % 10000;
    }

    if (ImGui::Button("Generate", ImVec2(200, 40)))
    {
        controller->Generate();
    }

    ImGui::End();
}

void TerrainUI::DisplayColorSettingsUI()
{
    ImGui::Begin("Color Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    // Use bold font for the main title
    ImGui::Spectrum::BeginTitleFont();
    ImGui::Text("Color Settings");
    ImGui::Spectrum::EndTitleFont();

    ImGui::Separator();
    ImGui::Spacing();

    for (size_t i = 0; i < parameters.colors.size(); i++)
    {
        ImGui::PushID(i);
        ImGui::Spectrum::BeginTitleFont();
        ImGui::Text("Color %d", i + 1);
        ImGui::Spectrum::EndTitleFont();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));        // Red background
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Lighter red on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // Dark red when clicked
        ImGui::SameLine();
        if (ImGui::Button("X"))
        {
            parameters.colors.erase(parameters.colors.begin() + i);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            break;
        }
        ImGui::PopStyleColor(3);

        if (i < parameters.colors.size() - 1)
        {
            ImGui::SameLine();
            if (ImGui::ArrowButton("Down", ImGuiDir_Down))
            {
                std::swap(parameters.colors[i], parameters.colors[i + 1]);
                std::swap(parameters.colors[i].height, parameters.colors[i + 1].height);
            }
        }
        if (i > 0)
        {
            ImGui::SameLine();
            if (ImGui::ArrowButton("Up", ImGuiDir_Up))
            {
                std::swap(parameters.colors[i], parameters.colors[i - 1]);
                std::swap(parameters.colors[i].height, parameters.colors[i - 1].height);
            }
        }

        ImGui::ColorEdit3("Color", &parameters.colors[i].color[0]);
        ImGui::SliderFloat("Height", &parameters.colors[i].height, 0.0f, 1.0f);

        ImGui::PopID();
        ImGui::Separator();
    }

    // Limit to 32 colors
    if (parameters.colors.size() < 32)
    {
        string addColorLabel = "Add Color (" + to_string(32 - parameters.colors.size()) + " left)";
        if (ImGui::Button(addColorLabel.c_str(), ImVec2(200, 40)))
        {
            TerrainUtilities::VertexColor defaultColor;
            defaultColor.color = {1.0f, 1.0f, 1.0f, 1.0f};
            defaultColor.height = 0.5f;
            parameters.colors.push_back(defaultColor);
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Maximum 32 colors reached!");
    }
}

void TerrainUI::DisplayTextureLayerSettings()
{
    ImGui::Begin("Texture Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Spectrum::BeginTitleFont();
    ImGui::Text("Terrain Texture Layers");
    ImGui::Spectrum::EndTitleFont();
    ImGui::Separator();

    for (int i = 0; i < parameters.loadedTextures.size(); ++i)
    {
        ImGui::PushID(i);

        // Use bold font for layer header
        ImGui::Spectrum::BeginTitleFont();
        ImGui::Text("Layer %d", i + 1);
        ImGui::Spectrum::EndTitleFont();

        if (i < parameters.loadedTextures.size() - 1)
        {
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Down", ImGuiDir_Down))
            {
                std::swap(parameters.loadedTextures[i], parameters.loadedTextures[i + 1]);
                std::swap(parameters.loadedTextures[i].height, parameters.loadedTextures[i + 1].height);
            }
        }
        if (i > 0)
        {
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Up", ImGuiDir_Up))
            {
                std::swap(parameters.loadedTextures[i], parameters.loadedTextures[i - 1]);
                std::swap(parameters.loadedTextures[i].height, parameters.loadedTextures[i - 1].height);
            }
        }

        ImGui::Text("Height");
        ImGui::SameLine();
        ImGui::SliderFloat("##Height", &parameters.loadedTextures[i].height, 0.0f, 1.0f, "%.2f");

        ImGui::Text("Tiling");
        ImGui::SameLine();
        ImGui::DragFloat2("##Tiling", glm::value_ptr(parameters.loadedTextures[i].tiling), 0.01f, 0.01f, 100.0f, "%.2f");

        ImGui::Text("Offset");
        ImGui::SameLine();
        ImGui::DragFloat2("##Offset", glm::value_ptr(parameters.loadedTextures[i].offset), 0.01f, -10.0f, 10.0f, "%.2f");

        ImGui::BeginGroup();
        {
            // Display texture image
            ImTextureID textureID = (ImTextureID)(intptr_t)parameters.loadedTextures[i].texture->ID;
            ImGui::Image(textureID, ImVec2(128, 128));
            ImGui::SameLine();

            // Controls next to the texture
            ImGui::BeginGroup();
            {
                // Show path with potential truncation if too long
                std::string path = parameters.loadedTextures[i].texture->path;
                if (path.length() > 40)
                {
                    path = "..." + path.substr(path.length() - 37);
                }

                // Position the text at the top for better layout
                ImGui::Dummy(ImVec2(0, 10)); // Add some padding at top
                ImGui::Text("Texture: %s", path.c_str());

                // Add space between text and buttons
                ImGui::Dummy(ImVec2(0, 40));

                // Button layout - fixed width but increased height
                const float buttonWidth = 75.0f;  // Fixed width similar to before
                const float buttonHeight = 33.0f; // Increased height to fit text

                // Style and position for Change button
                if (ImGui::Button("Change", ImVec2(buttonWidth, buttonHeight)))
                {
                    std::string file = Utils::FileDialogs::OpenFile(
                        "Select Texture",
                        "Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0",
                        Application::GetInstance()->GetWindow()->getNativeWindow());

                    if (!file.empty())
                    {
                        std::shared_ptr<Texture> newTexture = std::make_shared<Texture>(file.c_str());
                        parameters.loadedTextures[i].texture = newTexture;
                    }
                }

                // Put buttons side by side with a small gap
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(10, 0));
                ImGui::SameLine();

                // Style and position for Remove button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));

                if (ImGui::Button("Remove", ImVec2(buttonWidth, buttonHeight)))
                {
                    parameters.loadedTextures.erase(parameters.loadedTextures.begin() + i);
                    ImGui::EndGroup();
                    ImGui::EndGroup();
                    ImGui::PopID();
                    ImGui::PopStyleColor(3);
                    break;
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();

        ImGui::PopID();
        ImGui::Separator();
    }

    // Limit to 16 layers
    if (parameters.loadedTextures.size() < 16)
    {
        string addLayerLabel = "Add Layer (" + to_string(16 - parameters.loadedTextures.size()) + " left)";
        if (ImGui::Button(addLayerLabel.c_str(), ImVec2(200, 40)))
        {
            std::string file = Utils::FileDialogs::OpenFile("Select Texture", "Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0",
                                                            Application::GetInstance()->GetWindow()->getNativeWindow());

            if (!file.empty())
            {
                std::shared_ptr<Texture> texture = std::make_shared<Texture>(file.c_str());
                TerrainUtilities::TextureData layer = {texture, 0.5f};
                layer.tiling = glm::vec2(1.0f, 1.0f);
                layer.offset = glm::vec2(0.0f, 0.0f);
                parameters.loadedTextures.push_back(layer);
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Maximum 16 layers reached!");
    }
}

void TerrainUI::DisplayDecorationSettings()
{
    ImGui::Begin("Decoration Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::Spectrum::BeginTitleFont();
    ImGui::Text("Decoration Settings");
    ImGui::Spectrum::EndTitleFont();

    ImGui::Separator();
    for (size_t i = 0; i < parameters.decorationRules.size(); ++i)
    {
        ImGui::PushID(i);

        ImGui::Spectrum::BeginTitleFont();
        ImGui::Text("Decoration %zu", i + 1);
        ImGui::Spectrum::EndTitleFont();

        ImGui::DragFloat2("HeightLimits", glm::value_ptr(parameters.decorationRules[i].heightLimits), 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat2("ScaleRange", glm::value_ptr(parameters.decorationRules[i].scaleRange), 0.01f, 0.01f, 10.0f);

        ImGui::Checkbox("Random Rotation", &parameters.decorationRules[i].randomRotation);
        ImGui::DragFloat("Density", &parameters.decorationRules[i].density, 0.001f, 0.0f, 1.0f);

        ImGui::Text("Model Path");
        ImGui::SameLine();
        ImGui::Text("%s", parameters.decorationRules[i].modelPath.c_str());

        // TODO: refactor buttons dimensions to be in higer class
        // Button layout - fixed width but increased height
        const float buttonWidth = 75.0f;  // Fixed width similar to before
        const float buttonHeight = 33.0f; // Increased height to fit text

        ImGui::BeginGroup();
        {
            if (ImGui::Button("Change Model"))
            {
                std::string file = Utils::FileDialogs::OpenFile("Select Model", "Model Files (*.fbx;*.obj)\0*.fbx;*.obj\0",
                                                                Application::GetInstance()->GetWindow()->getNativeWindow());

                if (!file.empty())
                {
                    bool exists = false;
                    for (const auto &rule : parameters.decorationRules)
                    {
                        if (rule.modelPath == file && rule.modelPath != parameters.decorationRules[i].modelPath)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (exists)
                    {
                        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Model already exists in the list!");
                    }
                    else
                    {
                        parameters.decorationRules[i].modelPath = file;
                    }
                }
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            if (ImGui::Button("Remove", ImVec2(buttonWidth, buttonHeight)))
            {
                parameters.decorationRules.erase(parameters.decorationRules.begin() + i);
                ImGui::EndGroup();
                ImGui::PopID();
                ImGui::PopStyleColor(3);
                break;
            }
            ImGui::PopStyleColor(3);
            ImGui::EndGroup();
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    if (parameters.decorationRules.size() < 16)
    {
        string addLayerLabel = "Add Layer (" + to_string(16 - parameters.decorationRules.size()) + " left)";
        if (ImGui::Button(addLayerLabel.c_str(), ImVec2(200, 40)))
        {
            std::string file = Utils::FileDialogs::OpenFile("Select Model", "Model Files (*.fbx;*.obj)\0*.fbx;*.obj\0",
                                                            Application::GetInstance()->GetWindow()->getNativeWindow());

            if (!file.empty())
            {
                // Check if the file is already in the list
                bool exists = false;
                for (const auto &rule : parameters.decorationRules)
                {
                    if (rule.modelPath == file)
                    {
                        exists = true;
                        break;
                    }
                }

                if (exists)
                {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Model already exists in the list!");
                }
                else
                {
                    parameters.decorationRules.push_back(
                        {
                        glm::vec2(0.15f, 0.35f), // height limits
                        glm::vec2(0.8f, 1.2f),   // scale range
                        true,                    // random rotation
                        0.004f,                  // density
                        file
                        });
                }
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "Maximum 16 layers reached!");
    }
}

TerrainUtilities::TerrainData TerrainUI::GetParameters() const
{
    return parameters;
}