#include "SocketSystem.h"
#include <iostream>

void SocketSystem::Initialize() {

}

void SocketSystem::AddBlockTemplate(const BlockTemplate& blockTemplate) {
    blockTemplates[blockTemplate.blockId] = blockTemplate;
    std::cout << "Added block template for ID " << blockTemplate.blockId << std::endl;
}

void SocketSystem::GenerateRotatedVariants() {
    rotatedVariants.clear();
    
    for (const auto& [blockId, blockTemplate] : blockTemplates) {
        std::vector<RotatedBlockVariant> variants;
        
        for (int rotation : blockTemplate.allowedRotations) {
            RotatedBlockVariant variant(blockId, rotation);
            variant.rotatedSockets = RotateSockets(blockTemplate.sockets, rotation);
            variants.push_back(variant);
        }
        
        rotatedVariants[blockId] = variants;
        std::cout << "Generated " << variants.size() << " rotation variants for block " << blockId << std::endl;
    }
}

std::array<Socket, 6> SocketSystem::RotateSockets(const std::array<Socket, 6>& original, int yRotation) const {
    std::array<Socket, 6> rotated = original;
    
    // Face indices: [+X=0, -X=1, +Y=2, -Y=3, +Z=4, -Z=5]
    switch (yRotation) {
        case 90:
            rotated[0] = original[4]; // +X ← +Z
            rotated[1] = original[5]; // -X ← -Z  
            rotated[4] = original[1]; // +Z ← -X
            rotated[5] = original[0]; // -Z ← +X
            // Y faces (top/bottom) stay the same
            rotated[2] = original[2]; // +Y unchanged
            rotated[3] = original[3]; // -Y unchanged
            break;
            
        case 180:
            rotated[0] = original[1]; // +X ← -X
            rotated[1] = original[0]; // -X ← +X
            rotated[4] = original[5]; // +Z ← -Z
            rotated[5] = original[4]; // -Z ← +Z
            // Y faces stay the same
            rotated[2] = original[2];
            rotated[3] = original[3];
            break;
            
        case 270:
            rotated[0] = original[5]; // +X ← -Z
            rotated[1] = original[4]; // -X ← +Z
            rotated[4] = original[0]; // +Z ← +X
            rotated[5] = original[1]; // -Z ← -X
            // Y faces stay the same
            rotated[2] = original[2];
            rotated[3] = original[3];
            break;
            
        case 0:
        default:
            // No rotation
            break;
    }
    
    return rotated;
}

std::array<Socket, 6> SocketSystem::GetRotatedSockets(int blockId, int rotation) const {
    auto it = rotatedVariants.find(blockId);
    if (it == rotatedVariants.end()) {
        std::cerr << "No variants found for block " << blockId << std::endl;
        return {};
    }
    
    for (const auto& variant : it->second) {
        if (variant.rotationY == rotation) {
            return variant.rotatedSockets;
        }
    }
    
    std::cerr << "No variant found for block " << blockId << " with rotation " << rotation << std::endl;
    return {};
}

bool SocketSystem::CanBlocksConnect(int blockId1, int rotation1, int face1, 
                                   int blockId2, int rotation2, int face2,
                                   bool neighborIsEmpty) const {
    auto sockets1 = GetRotatedSockets(blockId1, rotation1);

    SocketType type2;
    if (neighborIsEmpty) {
        type2 = SocketType::EMPTY; // Treat as wildcard
    } else {
        auto sockets2 = GetRotatedSockets(blockId2, rotation2);
        type2 = sockets2[face2].type;
    }

    SocketType type1 = sockets1[face1].type;

    // If neighbor is empty, allow any connection
    if (neighborIsEmpty) {
        return true;
    }

    return compatibility.CanConnect(type1, type2);
}
