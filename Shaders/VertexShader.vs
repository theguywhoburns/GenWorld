#version 330 core

layout(location = 0) in vec3 aPos; // the position variable has attribute position 0
layout(location = 1) in vec3 aNormal;   // the normal variable has attribute position 1
layout(location = 3) in vec2 aTexCoords; // the texture variable has attribute position 2

out vec2 TexCoords; // output the texture coordinates to the fragment shader
out vec3 Normals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords; // set the output texture coordinate to the input texture coordinate
    Normals = mat3(transpose(inverse(model))) * aNormal;   // calculate the normals in world space
}
