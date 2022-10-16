#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "../base/texture2d.h"
#include "../base/texture_cubemap.h"

class Skybox {
public:
	// for IBL
	std::unique_ptr<TextureCubemap> irradianceMap;
	std::unique_ptr<TextureCubemap> prefilterMap;
	std::unique_ptr<Texture2D> brdfLutMap;

	float exposure = 4.5f;
	float gamma = 2.2f;
	float scaleIBLAmbient = 1.0f;
	float backgroundLod = 0.0f;

public:
	Skybox(
		const std::string& equirectImagePath,
		const std::string& equirectToCubemapVsFilepath,
		const std::string& equirectToCubemapFsFilepath, 
		const uint32_t resolution);

	Skybox(Skybox&& rhs) noexcept;

	~Skybox();

	void generateIrradianceMap(
		const std::string& irradianceConvolutionVsFilepath,
		const std::string& irradianceConvolutionFsFilepath,
		uint32_t resolution, 
		float deltaTheta,
		float deltaPhi);

	void generatePrefilterMap(
		const std::string& prefilterVsFilepath,
		const std::string& prefilterFsFilepath, 
		uint32_t resolution, 
		uint32_t numSamples);

	void generateBrdfLutMap(
		const std::string brdfLutVsFilepath,
		const std::string brdfLutFsFilepath,
		uint32_t resolution,
		uint32_t numSamples);

	void bindEnvironmentMap(int slot = 0) const;

	uint32_t getMaxPrefilterMipLevel() const;

	void draw() const ;

private:
	GLuint _vao = 0;
	GLuint _vbo = 0;

	// we'll use the naive handle here to prevent coupling with TextureCubemap 
	// in project 6, in which the students are asked to finish the class on their own.
	GLuint _texture = 0;

	uint32_t _maxPrefilteredMipLevel = 0;

	void createVertexResources();

	void equirectangulerToCubemap(
		const std::string& equirectImagePath,
		const std::string& equirectToCubemapVsFilepath,
		const std::string& equirectToCubemapFsFilepath,
		uint32_t resolution);

	void cleanup();

	static uint32_t nextPow2(uint32_t n);
};