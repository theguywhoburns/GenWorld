#pragma once

#include <string>
#include <vector>

struct MeshData {
    std::string name;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    MeshData(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
        : name(name), vertices(vertices), indices(indices) {}
};
