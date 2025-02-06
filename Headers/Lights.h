#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include <iostream>

class LightSource {
public:
	LightSource(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
		: ambient(ambient), diffuse(diffuse), specular(specular) {}

	virtual void Draw(Shader& shader);

protected:
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	std::string name;

};

class DirectionalLight : LightSource {
public:
	DirectionalLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
		: LightSource(ambient, diffuse, specular), direction(direction)
	{
		name = "dirLight";
	}

	void Draw(Shader& shader) override;

protected:
	glm::vec3 direction;

public:
	void SetDirection(glm::vec3 dir) { direction = dir; }

};

class SpotLight : LightSource {
public:
	SpotLight(int index, glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
		float cutOff, float outerCutOff, float constant, float linear, float quadratic)
		: LightSource(ambient, diffuse, specular), index(index), position(position), direction(direction), cutOff(cutOff), outerCutOff(outerCutOff),
		constant(constant), linear(linear), quadratic(quadratic)
	{
		name = "spotLights[" + std::to_string(index) + "]";
	}

	void Draw(Shader& shader) override;

protected:
	int index;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;
	float constant;
	float linear;
	float quadratic;

public:
	glm::vec3 position;
	void SetPosition(glm::vec3 pos) { position = pos; }
	void SetDirection(glm::vec3 dir) { direction = dir; }
	void SetCutOff(float cut) { cutOff = cut; }
	void SetOuterCutOff(float outerCut) { outerCutOff = outerCut; }
	void SetConstant(float con) { constant = con; }
	void SetLinear(float lin) { linear = lin; }
	void SetQuadratic(float quad) { quadratic = quad; }
	void SetIndex(int i) { index = i; }
	glm::vec3 GetColor() { return diffuse; }

};

class PointLight : LightSource {
public:
	PointLight(int index, glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
		float constant, float linear, float quadratic)
		: LightSource(ambient, diffuse, specular), index(index), position(position), constant(constant), linear(linear), quadratic(quadratic)
	{
		name = "pointLights[" + std::to_string(index) + "]";
	}

	void Draw(Shader& shader) override;

protected:
	int index;
	float constant;
	float linear;
	float quadratic;

public:
	glm::vec3 position;
	void SetPosition(glm::vec3 pos) { position = pos; }
	void SetConstant(float con) { constant = con; }
	void SetLinear(float lin) { linear = lin; }
	void SetQuadratic(float quad) { quadratic = quad; }
	void SetIndex(int i) { index = i; }
	glm::vec3 GetColor() { return diffuse; }

};