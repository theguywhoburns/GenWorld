#version 440 core

layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

out vec3 vertexColor;
out vec2 vertexTexCoord;

void main() {
    const vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0),  // Bottom-left (triangle 1)
        vec2(1.0, -1.0),   // Bottom-right (triangle 1)
        vec2(1.0, 1.0),    // Top-right (triangle 1)

        vec2(1.0, 1.0),    // Top-right (triangle 2)
        vec2(-1.0, 1.0),   // Top-left (triangle 2)
        vec2(-1.0, -1.0)   // Bottom-left (triangle 2)
    );

    // Make sure texture coordinates are properly aligned with screen-space positions
    // Converting from [-1,1] to [0,1] range for texture coordinates
    vec2 pos = positions[gl_VertexID];
    vertexTexCoord = vec2(
        (pos.x + 1.0) * 0.5,  // Convert from [-1,1] to [0,1]
        (pos.y + 1.0) * 0.5   // Convert from [-1,1] to [0,1]
    );

    vertexColor = aColor;
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0); 
}