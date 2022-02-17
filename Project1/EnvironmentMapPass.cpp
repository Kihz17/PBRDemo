#include "EnvironmentMapPass.h"
#include "FrameBuffer.h"
#include "TextureManager.h"
#include "Texture2D.h"
#include "ShaderLibrary.h"
#include "RenderBuffer.h"
#include "Renderer.h"

const std::string EnvironmentMapPass::CUBE_MAP_DRAW_SHADER_KEY = "drawEnvShader";
const std::string EnvironmentMapPass::CUBE_MAP_CONVERT_SHADER_KEY = "hdrToCubeShader";

EnvironmentMapPass::EnvironmentMapPass(const WindowSpecs* windowSpecs)
	: environmentBuffer(new FrameBuffer()),
	shader(ShaderLibrary::Load(CUBE_MAP_DRAW_SHADER_KEY, "assets/shaders/environmentBuffer.glsl")),
	cube(ShapeType::Cube),
	envMapHDR(nullptr),
	envMapCube(TextureManager::CreateCubeMap(1024, GL_LINEAR_MIPMAP_LINEAR, GL_RGB, GL_RGB16F, GL_FLOAT)),
	cubeMapBuffer(new FrameBuffer()),
	conversionShader(ShaderLibrary::Load(CUBE_MAP_CONVERT_SHADER_KEY, "assets/shaders/equirectangularToCubeMap.glsl")),
	windowSpecs(windowSpecs)
{
	// Setup color attachements for FBO
	environmentBuffer->Bind();
	environmentBuffer->AddColorAttachment2D("environment", TextureManager::CreateTexture2D(GL_RGBA16F, GL_RGBA, GL_FLOAT, windowSpecs->width, windowSpecs->height, TextureFilterType::Linear, TextureWrapType::None), 0);
	environmentBuffer->Unbind();

	// Setup shader uniforms
	shader->Bind();
	shader->InitializeUniform("uProjection");
	shader->InitializeUniform("uView");
	shader->InitializeUniform("uEnvMap");
	shader->SetInt("uEnvMap", 0);
	shader->Unbind();

	// Setup conversion shader uniforms
	conversionShader->Bind();
	conversionShader->InitializeUniform("uProjection");
	conversionShader->InitializeUniform("uView");
	conversionShader->InitializeUniform("uEnvMap");
	conversionShader->Unbind();
}

EnvironmentMapPass::~EnvironmentMapPass()
{
	delete environmentBuffer;
	delete cubeMapBuffer;
}

void EnvironmentMapPass::DoPass(const glm::mat4& projection, const glm::mat4& view)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE); // Make sure none of our cubes faces get culled

	environmentBuffer->Bind();
	shader->Bind();
	shader->SetMat4("uProjection", projection);
	shader->SetMat4("uView", view);
	envMapCube->BindToSlot(0);
	shader->SetInt("uEnvMap", 0);
	cube.Draw();
	environmentBuffer->Unbind();

	glEnable(GL_CULL_FACE); // Re-enable face culling
}

void EnvironmentMapPass::SetEnvironmentMapEquirectangular(const std::string& path)
{
	envMapHDR = TextureManager::CreateTexture2D(path, TextureFilterType::Linear, TextureWrapType::Repeat, true, true, true); // new HDR env map

	glDisable(GL_CULL_FACE); // Make sure none of our cubes faces get culled

	// Convert the HDR equirectangular texture to its cube map equivalent
	{
		// Setup conversion uniforms for shader
		const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		const glm::mat4 captureViews[] = // 6 different view matrices that face each side of the cube
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
		};

		conversionShader->Bind();
		conversionShader->SetMat4("uProjection", captureProjection);

		envMapHDR->BindToSlot(0);

		glViewport(0, 0, envMapCube->GetWidth(), envMapCube->GetHeight()); // Make sure we configure the viewport to capture the texture dimensions

		cubeMapBuffer->Bind();
		for (int i = 0; i < 6; i++) // Setup all faces of the cube
		{
			conversionShader->SetMat4("uView", captureViews[i]);
			cubeMapBuffer->AddColorAttachmentCubeMapFace("cubeFace", envMapCube, 0, (CubeMapFace)i);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cube.Draw();
		}

		envMapCube->ComputeMipmap();
		cubeMapBuffer->Unbind();
	}

	glViewport(0, 0, windowSpecs->width, windowSpecs->height); // Set viewport back to native width/height
	glEnable(GL_CULL_FACE); // Re-enable face culling
}