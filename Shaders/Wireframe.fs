#version 440 core

out vec4 FragColor;

in FragData {
    noperspective vec3 wireframeDist;
} fragData;

uniform vec4 modelColor = vec4(1.0);
uniform vec4 wireframeColor = vec4(0.0, 0.0, 0.0, 1.0);

uniform float width = 0.5;

void main() {
    vec3 deltas = fwidth(fragData.wireframeDist);
    vec3 barys_s = smoothstep(deltas * width, deltas * width, fragData.wireframeDist);
    float wires = min(barys_s.x, min(barys_s.y, barys_s.z));

    FragColor = mix(wireframeColor, modelColor, wires);
}