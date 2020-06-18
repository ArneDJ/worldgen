#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../external/fastnoise/FastNoise.h"

#include "../graphics/imp.h"
#include "terraform.h"

static struct floatimage gen_heightmap(size_t side, long seed, float freq)
{
	struct floatimage image = {
		.data = new float[side*side],
		.nchannels = RED_CHANNEL,
		.width = side,
		.height = side,
	};

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(freq);
	noise.SetFractalOctaves(6);
	noise.SetFractalLacunarity(2.2f);
	noise.SetGradientPerturbAmp(300.f);

	const glm::vec2 center = glm::vec2(0.5f*float(image.width), 0.5f*float(image.height));

	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(1.5f*freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Div);
	cellnoise.SetGradientPerturbAmp(200.f);

	float max = 1.f;
	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			float x = i; float y = j;
			noise.GradientPerturbFractal(x, y);

			float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
			height = glm::clamp(height, 0.f, 1.f);

			float mask = glm::distance(0.5f*float(image.width), float(y)) / (0.5f*float(image.width));
			mask = 1.f - glm::clamp(mask, 0.f, 1.f);
			mask = glm::smoothstep(0.2f, 0.5f, sqrtf(mask));
			height *= sqrtf(mask);

			x = i; y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float ridge = cellnoise.GetNoise(x, y) / max;

			height = glm::mix(height, ridge, ridge*glm::smoothstep(0.4f, 0.7f, height));

			image.data[index++] = height;
		}
	}

	return image;
}

static struct byteimage gen_temperature(size_t side, long seed)
{
	struct byteimage image = {
		.data = new unsigned char[side*side],
		.nchannels = RED_CHANNEL,
		.width = side,
		.height = side,
	};

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(0.002f);
	noise.SetGradientPerturbAmp(400.f);

	const float height = float(image.height);
	unsigned int index = 0;
	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			float y = i; float x = j;
			noise.GradientPerturbFractal(x, y);
			float lattitude = 1.f - (y / height);
			image.data[index++] = 255.f * glm::clamp(lattitude, 0.f, 1.f);
		}
	}

	return image;
}

static struct byteimage gen_rainfall(const struct floatimage *heightimage, const struct byteimage *tempimage, long seed)
{
	const size_t size = heightimage->width * heightimage->height * heightimage->nchannels;

	struct byteimage image = {
		.data = new unsigned char[size],
		.nchannels = heightimage->nchannels,
		.width = heightimage->width,
		.height = heightimage->height,
	};

	for (auto i = 0; i < size; i++) {
		float height = heightimage->data[i];
		height = height < 0.5f ? 0.f : 1.f;
		image.data[i] = 255.f * (1.f - height);
	}

	gauss_blur_image(&image, 50.f);

	for (auto i = 0; i < size; i++) {
		float temperature = 1.f - (tempimage->data[i] / 255.f);
		float rainfall = image.data[i] / 255.f;
		rainfall = glm::mix(rainfall, temperature, 1.f - temperature);
		image.data[i] = 255.f * rainfall;
	}

	return image;
}

Terraform::Terraform(size_t side, long seed, float freq)
{
	heightmap = gen_heightmap(side, seed, freq);		

	temperature = gen_temperature(side, seed);
	rainfall = gen_rainfall(&heightmap, &temperature, seed);
}

Terraform::~Terraform(void) 
{
	if (heightmap.data != nullptr) {
		delete [] heightmap.data;
	}
	if (temperature.data != nullptr) {
		delete [] temperature.data;
	}
	if (rainfall.data != nullptr) {
		delete [] rainfall.data;
	}
}
