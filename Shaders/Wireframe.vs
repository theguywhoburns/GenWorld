#version 440 core

layout(location = 0) in vec3 aPos;

layout(location = 8) in vec4 instanceModel0;
layout(location = 9) in vec4 instanceModel1;
layout(location = 10) in vec4 instanceModel2;
layout(location = 11) in vec4 instanceModel3;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 WorldPos;

void main() {
    mat4 model = uModel;
    if (gl_InstanceID > 0) {
        model = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);
    }

    gl_Position = uProjection * uView * model * vec4(aPos, 1.0);
    WorldPos = vec3(model * vec4(aPos, 1.0));
}