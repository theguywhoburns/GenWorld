#version 440 core

out vec4 FragColor;

in FragData {
    noperspective vec3 wireframeDist;
} fragData;

uniform float wireframeWidth = 0.5;
uniform vec3 wireframeColor = vec3(0.0, 0.0, 0.0);

uniform bool useFill = true;
uniform vec3 fillColor = vec3(1.0);

void main() {
    vec3 deltas = fwidth(fragData.wireframeDist);
    vec3 barys_s = smoothstep(vec3(0.0), deltas * wireframeWidth, fragData.wireframeDist);
    float wires = min(barys_s.x, min(barys_s.y, barys_s.z));

    if(useFill) {
        // Mix wireframe and fill colors
        FragColor = vec4(mix(wireframeColor, fillColor, wires), 1.0);
    } else {
        // Only show wireframe, discard fill areas
        if(wires > 0.5) {
            discard;
        }
        FragColor = vec4(wireframeColor, 1.0);
    }
}