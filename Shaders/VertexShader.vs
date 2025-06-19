#version 330 core

layout(location = 0) in vec3 aPos; // the position variable has attribute position 0
layout(location = 1) in vec3 aNormal;   // the normal variable has attribute position 1
layout(location = 3) in vec2 aTexCoords; // the texture variable has attribute position 2

out vec2 TexCoords; // output the texture coordinates to the fragment shader
out vec3 Normals;

layout(location = 8) in vec4 instanceModel0;
layout(location = 9) in vec4 instanceModel1;
layout(location = 10) in vec4 instanceModel2;
layout(location = 11) in vec4 instanceModel3;

uniform mat4 view;
uniform mat4 projection;

void main() {
    mat4 model = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords; // set the output texture coordinate to the input texture coordinate
    Normals = mat3(transpose(inverse(model))) * aNormal;   // calculate the normals in world space
}