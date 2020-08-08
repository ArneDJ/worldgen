#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "FastNoise.h"

#include "imp.h"
#include "terra.h"

#define TEMP_RAIN_INFLUENCE 0.6f

static struct floatimage heightimage(size_t imageres, long seed, struct worldparams params)
{
	struct floatimage image = {
		.data = new float[imageres*imageres],
		.nchannels = 1,
		.width = imageres,
		.height = imageres,
	};

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(params.frequency);
	noise.SetPerturbFrequency(params.perturbfreq);
	noise.SetFractalOctaves(params.octaves);
	noise.SetFractalLacunarity(params.lacunarity);
	noise.SetGradientPerturbAmp(params.perturbamp);

	unsigned int index = 0;
	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			float x = j; float y = i;
			noise.GradientPerturbFractal(x, y);
			float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
			image.data[index++] = glm::clamp(height, 0.f, 1.f);
		}
	}

	return image;
}

static struct floatimage tempimage(size_t imageres, long seed, float freq, float perturb)
{
	struct floatimage image = {
		.data = new float[imageres*imageres],
		.nchannels = 1,
		.width = imageres,
		.height = imageres,
	};

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(freq);
	noise.SetPerturbFrequency(2.f*freq);
	noise.SetGradientPerturbAmp(perturb);

	const float longitude = float(image.height);
	unsigned int index = 0;
	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			float y = i; float x = j;
			noise.GradientPerturbFractal(x, y);
			float temperature = 1.f - (y / longitude);
			image.data[index++] = glm::clamp(temperature, 0.f, 1.f);
		}
	}

	return image;
}


static struct byteimage rainimage(const struct floatimage *elevation, const struct floatimage *temperature, float sealevel, float blur)
{
	const size_t size = elevation->width * elevation->height;

	struct byteimage image = {
		.data = new unsigned char[size],
		.nchannels = elevation->nchannels,
		.width = elevation->width,
		.height = elevation->height,
	};
	for (int i = 0; i < elevation->width*elevation->height; i++) {
		float h = elevation->data[i];
		if (h > sealevel) { 
			h = 1.f; 
		} else {
			h = 0.f;
		}
		image.data[i] = h * 255;
	}

	gauss_blur_image(&image, blur);

	for (auto i = 0; i < size; i++) {
		float temp = 1.f - temperature->data[i];
		float rain = image.data[i] / 255.f;
		rain = 1.f - rain;
		rain = glm::mix(rain, temp, TEMP_RAIN_INFLUENCE*(1.f - temp));
		image.data[i] = rain * 255;
	}

	return image;
}

Terraform::Terraform(size_t imageres, long seed, struct worldparams params)
{
	heightmap = heightimage(imageres, seed, params);
	tempmap = tempimage(imageres, seed, params.tempfreq, params.tempperturb);
	rainmap = rainimage(&heightmap, &tempmap, params.lowland, 50.f);
}

Terraform::~Terraform(void) 
{
	delete_floatimage(&heightmap);
	delete_floatimage(&tempmap);
	delete_byteimage(&rainmap);
}
