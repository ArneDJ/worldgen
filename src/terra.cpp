#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "extern/FastNoise.h"

#include "imp.h"
#include "terra.h"

#define RAIN_FREQUENCY 0.01F
#define RAIN_OCTAVES 6
#define RAIN_LACUNARITY 3.F
#define RAIN_PERTURB_FREQUENCY 0.01F
#define RAIN_PERTURB_AMP 50.F
#define RAIN_GAUSS_CENTER 0.25F
#define RAIN_GAUSS_SIGMA 0.25F
#define RAIN_DETAIL_MIX 0.5F

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
			float x = 4.f*j; float y = 4.f*i;
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

// The parameter a is the height of the curve's peak, b is the position of the center of the peak and c (the standard deviation, sometimes called the Gaussian RMS width) controls the width of the "bell".
static inline float gauss(float a, float b, float c, float x)
{
	float exponent = ((x-b)*(x-b)) / (2.f * (c*c));

	return a * std::exp(-exponent);
}

static struct byteimage rainimage(const struct byteimage *elevation, const struct byteimage *temperature, long seed, float sealevel, float blur)
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

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(RAIN_FREQUENCY);
	noise.SetFractalOctaves(RAIN_OCTAVES);
	noise.SetFractalLacunarity(RAIN_LACUNARITY);
	noise.SetPerturbFrequency(RAIN_PERTURB_FREQUENCY);
	noise.SetGradientPerturbAmp(RAIN_PERTURB_AMP);

	for (int i = 0; i < image.width; i++) {
		for (int j = 0; j < image.height; j++) {
			int index = i * image.width + j;
			float temp = 1.f - (temperature->data[index] / 255.f);
			float rain = 1.f - (image.data[index] / 255.f);
			float y = i; float x = j;
			noise.GradientPerturbFractal(x, y);
			float detail = (noise.GetNoise(x, y) + 1.f) / 2.f;
			float dev = gauss(1.f, RAIN_GAUSS_CENTER, RAIN_GAUSS_SIGMA, rain);
			rain = glm::mix(rain, detail, RAIN_DETAIL_MIX*dev);
			rain = glm::mix(rain, temp, detail*(1.f - temp));
			image.data[index] = glm::clamp(rain, 0.f, 1.f) * 255;
		}
	}

	return image;
}

struct terraform form_terra(size_t imageres, long seed, struct worldparams params)
{
	struct terraform terra;
	terra.heightmap = heightimage(imageres, seed, params);
	terra.tempmap = tempimage(imageres, seed, params.tempfreq, params.tempperturb);
	terra.rainmap = rainimage(&terra.heightmap, &terra.tempmap, seed, params.lowland, params.rainblur);

	return terra;
}
