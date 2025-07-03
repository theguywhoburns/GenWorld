#pragma once
#include <vector>
#include <map>
#include <array>
#include <string>

enum class SocketType {
    EMPTY = 0,      // Can connect to anything (like air)
    GRASS = 1,      // Grass blocks
    STONE = 2,      // Stone blocks  
    WOOD = 3,       // Wooden blocks
    METAL = 4,      // Metal blocks
    WALL = 5,       // Wall blocks (cannot connect to anything)
    CUSTOM_1 = 6,   // User-defined types
    CUSTOM_2 = 7,
    CUSTOM_3 = 8,
    CUSTOM_4 = 9,
    CUSTOM_5 = 10,
};

struct Socket {
    SocketType type;
    bool symmetric;     // If true, can connect in any orientation
    
    Socket(SocketType t = SocketType::EMPTY, bool sym = true) 
        : type(t), symmetric(sym) {}

};

struct BlockTemplate {
    int blockId;
    std::string name;
    std::array<Socket, 6> sockets; // [+X, -X, +Y, -Y, +Z, -Z]
    std::vector<int> allowedRotations; // [0, 90, 180, 270] degrees
    BlockTemplate(int id = 0) : blockId(id) {
        // Default: all sockets are EMPTY and symmetric
        sockets.fill(Socket(SocketType::EMPTY, true));
        allowedRotations = {0, 90, 180, 270}; // Allow all rotations by default
    }
};

struct RotatedBlockVariant {
    int baseBlockId;
    int rotationY;      // 0, 90, 180, 270 degrees
    std::array<Socket, 6> rotatedSockets;
    
    RotatedBlockVariant(int id = 0, int rot = 0) : baseBlockId(id), rotationY(rot) {
        rotatedSockets.fill(Socket());
    }
};

class SocketCompatibility {
private:
    std::map<std::pair<SocketType, SocketType>, bool> connectionRules;
    
public:
    void AddRule(SocketType socketA, SocketType socketB, bool canConnect) {
        connectionRules[{socketA, socketB}] = canConnect;
        connectionRules[{socketB, socketA}] = canConnect; // Symmetric
    }
    
    bool CanConnect(SocketType socketA, SocketType socketB) const {
        // EMPTY sockets can connect to anything
        if (socketA == SocketType::EMPTY || socketB == SocketType::EMPTY) {
            return true;
        }
        
        // WALL sockets cannot connect to anything (they represent exterior faces)
        if (socketA == SocketType::WALL || socketB == SocketType::WALL) {
            return false;
        }
        
        auto it = connectionRules.find({socketA, socketB});
        
        return it != connectionRules.end() ? it->second : false;
    }
    

    void ClearAllRules() {
        connectionRules.clear();
    }
};

class SocketSystem {
private:
    std::map<int, BlockTemplate> blockTemplates;
    std::map<int, std::vector<RotatedBlockVariant>> rotatedVariants;
    SocketCompatibility compatibility;
    
public:
    void Initialize();
    void AddBlockTemplate(const BlockTemplate& blockTemplate);
    void GenerateRotatedVariants();
    
    bool CanBlocksConnect(int blockId1, int rotation1, int face1, 
                         int blockId2, int rotation2, int face2,
                         bool neighborIsEmpty) const;

    const char* SocketTypeToString(SocketType type) const{
        switch(type) {
            case SocketType::CUSTOM_1: return "Custom1";
            case SocketType::CUSTOM_2: return "Custom2";
            case SocketType::CUSTOM_3: return "Custom3";
            case SocketType::CUSTOM_4: return "Custom4";
            case SocketType::CUSTOM_5: return "Custom5";
            case SocketType::METAL:   return "Metal";
            case SocketType::STONE:   return "Stone";
            case SocketType::WOOD:    return "Wood";
            case SocketType::GRASS:   return "Grass";
            case SocketType::WALL:    return "Wall";
            case SocketType::EMPTY:   return "Empty";
            default:                  return "Invalid";
        }
    }
    std::array<Socket, 6> GetRotatedSockets(int blockId, int rotation) const;
    std::array<Socket, 6> RotateSockets(const std::array<Socket, 6>& original, int yRotation) const;
    
    const std::map<int, BlockTemplate>& GetBlockTemplates() const { return blockTemplates; }
    SocketCompatibility& GetCompatibility() { return compatibility; }
};