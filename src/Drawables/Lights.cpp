#include "Lights.h"

void LightSource::Draw(Shader& shader)
{
	shader.setVec3(name + ".ambient", ambient);
	shader.setVec3(name + ".diffuse", diffuse);
	shader.setVec3(name + ".specular", specular);
}

void DirectionalLight::Draw(Shader& shader)
{
	LightSource::Draw(shader);
	shader.setVec3(name + ".direction", direction);
}

void SpotLight::Draw(Shader& shader)
{
	LightSource::Draw(shader);
	shader.setVec3(name + ".position", position);
	shader.setVec3(name + ".direction", direction);
	shader.setFloat(name + ".cutOff", cutOff);
	shader.setFloat(name + ".outerCutOff", outerCutOff);
	shader.setFloat(name + ".constant", constant);
	shader.setFloat(name + ".linear", linear);
	shader.setFloat(name + ".quadratic", quadratic);
}

void PointLight::Draw(Shader& shader)
{
	LightSource::Draw(shader);
	shader.setVec3(name + ".position", position);
	shader.setFloat(name + ".constant", constant);
	shader.setFloat(name + ".linear", linear);
	shader.setFloat(name + ".quadratic", quadratic);
}
