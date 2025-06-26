#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normals;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
};

uniform sampler2D diffuse1;
// uniform Light light;
Light light;
uniform bool useLights = true;

vec3 CalcDirLight(vec3 normal, vec3 diffTex) {
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // combine results
    vec3 ambient = light.ambient * diffTex;
    vec3 diffuse = light.diffuse * diff * diffTex;

    return (ambient + diffuse);
}

void main()
{
    vec3 vertexNormal = normalize(Normals);
    vec4 finalColor = texture(diffuse1, TexCoords);

    if (useLights) {
		//TODO test values: set the uniform from the application UI
		light.direction = vec3(-1.0, -1.0, -0.5);
        light.ambient = vec3(0.5);
        light.diffuse = vec3(1.0);
        
        // calculate lighting
        vec3 normal = normalize(vertexNormal);
        vec3 lightColor = CalcDirLight(normal, finalColor.rgb);
        finalColor.rgb = lightColor;
    }

    FragColor = finalColor;
}
