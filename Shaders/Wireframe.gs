// Geometry shader for solid wireframe rendering
#version 330

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

out FragData {
    noperspective vec3 wireframeDist;
} outData;

void main() {
    for(int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;


        // This is the easiest scheme I could think of. The attribute will be interpolated, so
        // all you have to do is set the ith dimension to 1.0 to get barycentric coordinates 
        // specific to this triangle. The frag shader will interpolate and then you can just use
        // a threshold in the frag shader to figure out if you're close to an edge
        outData.wireframeDist = vec3(0.0);
        outData.wireframeDist[i] = 1.0;

        EmitVertex();
    }
}