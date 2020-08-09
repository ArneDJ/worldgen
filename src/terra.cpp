#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "extern/FastNoise.h"

#include "imp.h"
#include "terra.h"

static struct byteimage heightimage(size_t imageres, long seed, struct worldparams params)
{
	struct byteimage image = blank_byteimage(1, imageres, imageres);

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
			image.data[index++] = 255 * glm::clamp(height, 0.f, 1.f);
		}
	}

	return image;
}

static struct byteimage tempimage(size_t imageres, long seed, float freq, float perturb)
{
	struct byteimage image = blank_byteimage(1, imageres, imageres);

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
			image.data[index++] = 255 * glm::clamp(temperature, 0.f, 1.f);
		}
	}

	return image;
}

static struct byteimage rainimage(const struct byteimage *elevation, const struct byteimage *temperature, float tempinfluence, float sealevel, float blur)
{
	struct byteimage image = blank_byteimage(1, elevation->width, elevation->height);

	for (int i = 0; i < elevation->width*elevation->height; i++) {
		float h = elevation->data[i] / 255.f;
		if (h > sealevel) { 
			h = 1.f; 
		} else {
			h = 0.f;
		}
		image.data[i] = h * 255;
	}

	gauss_blur_image(&image, blur);

	for (auto i = 0; i < elevation->width*elevation->height; i++) {
		float temp = 1.f - (temperature->data[i] / 255.f);
		float rain = image.data[i] / 255.f;
		rain = 1.f - rain;
		rain = glm::mix(rain, temp, tempinfluence*(1.f - temp));
		image.data[i] = rain * 255;
	}

	return image;
}

Terraform::Terraform(size_t imageres, long seed, struct worldparams params)
{
	heightmap = heightimage(imageres, seed, params);
	tempmap = tempimage(imageres, seed, params.tempfreq, params.tempperturb);
	rainmap = rainimage(&heightmap, &tempmap, params.tempinfluence, params.lowland, params.rainblur);
}

Terraform::~Terraform(void) 
{
	delete_byteimage(&heightmap);
	delete_byteimage(&tempmap);
	delete_byteimage(&rainmap);
}
