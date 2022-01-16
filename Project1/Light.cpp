#include "Light.h"
#include "ShaderLibrary.h"
#include "Renderer.h"

#include <iostream>
#include <sstream>

int Light::currentLightIndex = 0;
glm::vec3 Light::orientation = glm::vec3(0.0f);
glm::vec3 Light::scale = glm::vec3(1.0f, 1.0f, 1.0f);

glm::vec3 LightRadius::orientation = glm::vec3(0.0f);

std::vector<int> Light::removedLights;

Light::Light(glm::vec3 pos, const glm::vec3& direction, const glm::vec3& color, LightType lightType, float radius, AttenuationMode attenMode, bool on, float intensity)
	: position(pos),
	direction(direction),
	color(color),
	lightType(lightType),
	radius(LightRadius(&pos, radius)),
	attenuationMode(attenMode),
	on(on),
	intensity(intensity)
{
	if (Light::currentLightIndex >= 1000 && removedLights.empty())
	{
		std::cout << "Cannot create more than 1000 lights!" << std::endl;
		return;
	}

	if (!removedLights.empty())
	{
		this->lightIndex = Light::removedLights[0];
		removedLights.erase(removedLights.begin());
	}
	else
	{
		this->lightIndex = Light::currentLightIndex++;
	}

	// Initialize uniforms
	std::stringstream ss;
	ss << "uLightArray[" << this->lightIndex << "].";
	std::string lightHandle = ss.str();

	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	positionLoc = std::string(lightHandle + "position");
	directionLoc = std::string(lightHandle + "direction");
	colorLoc = std::string(lightHandle + "color");
	param1Loc = std::string(lightHandle + "param1");

	SendToShader();

	brdfShader->Bind();
	brdfShader->SetInt("uLightAmount", Light::currentLightIndex + 1);

	forwardShader->Bind();
	forwardShader->SetInt("uLightAmount", Light::currentLightIndex + 1);
}

Light::~Light()
{
	UpdateOn(false); // Turn the light off in the shader
	removedLights.push_back(this->lightIndex); // Save the index so it can be used again later
}

void Light::UpdatePosition(const glm::vec3& position)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->position = position;

	brdfShader->Bind();
	brdfShader->SetFloat3(positionLoc, position);

	forwardShader->Bind();
	forwardShader->SetFloat3(positionLoc, position);
}

void Light::UpdateDirection(const glm::vec3& direction)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->direction = direction;

	brdfShader->Bind();
	brdfShader->SetFloat3(directionLoc, direction);

	forwardShader->Bind();
	forwardShader->SetFloat3(directionLoc, direction);
}

void Light::UpdateColor(const glm::vec3& color)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->color = color;

	brdfShader->Bind();
	brdfShader->SetFloat4(colorLoc, glm::vec4(color, intensity));

	forwardShader->Bind();
	forwardShader->SetFloat4(colorLoc, glm::vec4(color, intensity));
}

void Light::UpdateIntenisty(float intensity)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->intensity = intensity;

	brdfShader->Bind();
	brdfShader->SetFloat4(colorLoc, glm::vec4(color, intensity));

	forwardShader->Bind();
	forwardShader->SetFloat4(colorLoc, glm::vec4(color, intensity));
}

void Light::UpdateRadius(float radius)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->radius.UpdateRadius(radius);

	brdfShader->Bind();
	brdfShader->SetFloat4(param1Loc, glm::vec4((GLfloat) lightType, radius, (GLfloat) on, (GLfloat) attenuationMode));

	forwardShader->Bind();
	forwardShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius, (GLfloat)on, (GLfloat)attenuationMode));
}

void Light::UpdateOn(bool on)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->on = on;

	brdfShader->Bind();
	brdfShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));

	forwardShader->Bind();
	forwardShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));
}

void Light::UpdateAttenuationMode(AttenuationMode attenMode)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->attenuationMode = attenMode;

	brdfShader->Bind();
	brdfShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat) attenuationMode));

	forwardShader->Bind();
	forwardShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));
}

void Light::UpdateLightType(LightType lightType)
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	this->lightType = lightType;

	brdfShader->Bind();
	brdfShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));

	forwardShader->Bind();
	forwardShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));
}

void Light::SendToShader() const
{
	const Shader* brdfShader = ShaderLibrary::Get(Renderer::LIGHTING_SHADER_KEY);
	const Shader* forwardShader = ShaderLibrary::Get(Renderer::FORWARD_SHADER_KEY);

	brdfShader->Bind();
	brdfShader->SetFloat3(positionLoc, position);
	brdfShader->SetFloat3(directionLoc, direction);
	brdfShader->SetFloat4(colorLoc, glm::vec4(color, intensity));
	brdfShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));

	forwardShader->Bind();
	forwardShader->SetFloat3(positionLoc, position);
	forwardShader->SetFloat3(directionLoc, direction);
	forwardShader->SetFloat4(colorLoc, glm::vec4(color, intensity));
	forwardShader->SetFloat4(param1Loc, glm::vec4((GLfloat)lightType, radius.radius, (GLfloat)on, (GLfloat)attenuationMode));
}