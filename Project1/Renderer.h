#pragma once

#include "Framebuffer.h"
#include "Mesh.h"
#include "Camera.h"
#include "PrimitiveShape.h"
#include "Components.h"

#include <vector>

struct WindowSpecs
{
	GLFWwindow* window;
	int width, height;
};

struct RenderSubmission
{
	RenderComponent* renderComponent = nullptr;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

class Renderer
{
public:
	static void Initialize(WindowSpecs* window);
	static void CleanUp();

	static void SetViewType(uint32_t type);

	static void SetEnvironmentMapEquirectangular(const std::string& path);

	static void DrawFrame();

	static void BeginFrame(const Camera& camera);
	static void EndFrame();

	static void Submit(const RenderSubmission& submission);

	static const std::string G_SHADER_KEY;
	static const std::string LIGHTING_SHADER_KEY;
	static const std::string CUBE_MAP_CONVERT_SHADER_KEY;
	static const std::string IRRADIANCE_SHADER_KEY;
	static const std::string PREFILTER_SHADER_KEY;
	static const std::string ENV_LUT_SHADER_KEY;
	static const std::string CUBE_MAP_DRAW_SHADER_KEY;
	static const std::string FORWARD_SHADER_KEY;
	static const std::string GRASS_SHADER_KEY;

private:
	static void GeometryPass();
	static void EnvironmentPass();
	static void LightingPass();
	static void ForwardPass();

	static WindowSpecs* windowDetails;

	static Framebuffer* geometryBuffer;
	static Framebuffer* environmentBuffer;
	static Framebuffer* cubeMapBuffer;
	static Framebuffer* irradianceBuffer;
	static Framebuffer* envPrefilterBuffer;
	static Framebuffer* envLUTBuffer;

	// Environment map stuff
	static Texture* envMap;
	static CubeMap* envMapCube;
	static CubeMap* envMapIrradiance;
	static CubeMap* envMapPreFilter;

	static glm::mat4 projection;
	static glm::mat4 view;
	static glm::vec3 cameraPos;

	static PrimitiveShape* quad;
	static PrimitiveShape* cube;
};