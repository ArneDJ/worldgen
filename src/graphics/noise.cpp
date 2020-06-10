#include <iostream>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include "../external/fastnoise/FastNoise.h"

#include "imp.h"
#include "noise.h"

void billow_3D_image(unsigned char *image, size_t sidelength, float frequency, float cloud_distance)
{
	FastNoise billow;
	billow.SetNoiseType(FastNoise::SimplexFractal);
	billow.SetFractalType(FastNoise::Billow);
	billow.SetFrequency(frequency);
	billow.SetFractalOctaves(6);
	billow.SetFractalLacunarity(2.0f);

	const float space = cloud_distance; // space between the clouds

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			for (int k = 0; k < sidelength; k++) {
				float p = (billow.GetNoise(i, j, k) + 1.f) / 2.f;
				p = p - space;
				image[index++] = glm::clamp(p, 0.f, 1.f) * 255.f;
			}
		}
	}

}

void cellnoise_image(float *image, size_t sidelength, long seed, float freq)
{
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Add);
	cellnoise.SetGradientPerturbAmp(10.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float height = cellnoise.GetNoise(x, y) / max;
			image[index++] = height;
		}
	}
}

void badlands_image(float *image, size_t sidelength, long seed, float freq)
{
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Sub);
	cellnoise.SetGradientPerturbAmp(10.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float height = cellnoise.GetNoise(x, y) / max;
			height = sqrtf(height);
			if (height > 0.1f && height < 0.2f) { 
				height = glm::mix(0.1f, 0.2f, glm::smoothstep(0.15f, 0.16f, height));
			}
			if (height > 0.25f && height < 0.5f) { 
				height = glm::mix(0.25f, 0.5f, glm::smoothstep(0.35f, 0.4f, height));
			}
			image[index++] = height*height;
		}
	}
}

void terrain_image(float *image, size_t sidelength, long seed, float freq, float mountain_amp, float field_amp)
{
	// detail
	FastNoise billow;
	billow.SetSeed(seed);
	billow.SetNoiseType(FastNoise::SimplexFractal);
	billow.SetFractalType(FastNoise::Billow);
	billow.SetFrequency(0.01f);
	billow.SetFractalOctaves(6);
	billow.SetFractalLacunarity(2.0f);
	billow.SetGradientPerturbAmp(40.f);

	// ridges
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(0.01f*freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Add);
	cellnoise.SetGradientPerturbAmp(30.f);

	// mask perturb
	FastNoise perturb;
	perturb.SetSeed(seed);
	perturb.SetNoiseType(FastNoise::SimplexFractal);
	perturb.SetFractalType(FastNoise::FBM);
	perturb.SetFrequency(0.002f*freq);
	perturb.SetFractalOctaves(5);
	perturb.SetFractalLacunarity(2.0f);
	perturb.SetGradientPerturbAmp(300.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	const glm::vec2 center = glm::vec2(0.5f*float(sidelength), 0.5f*float(sidelength));
	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			billow.GradientPerturbFractal(x, y);
			float detail = 1.f - (billow.GetNoise(x, y) + 1.f) / 2.f;

			x = i; y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float ridge = cellnoise.GetNoise(x, y) / max;

			x = i; y = j;
			perturb.GradientPerturbFractal(x, y);

			// add detail
			float height = glm::mix(detail, ridge, 0.9f);

			// aply mask
			float mask = glm::distance(center, glm::vec2(float(x), float(y))) / float(0.5f*sidelength);
			mask = glm::smoothstep(0.4f, 0.8f, mask);
			mask = glm::clamp(mask, field_amp, mountain_amp);

			height *= mask;

			if (i > (sidelength-4) || j > (sidelength-4)) {
				image[index++] = 0.f;
			} else {
				//image[index++] = height * 255.f;
				image[index++] = height;
			}
		}
	}
}

void simplex_image(struct floatimage *image, long seed, float frequency, float perturb)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(frequency);
	noise.SetFractalOctaves(5);
	noise.SetGradientPerturbAmp(perturb);

	const glm::vec2 center = glm::vec2(0.5f*float(image->width), 0.5f*float(image->height));

	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(1.5f*frequency);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Div);
	cellnoise.SetGradientPerturbAmp(100.f);

	float max = 1.f;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float x = i; float y = j;
			noise.GradientPerturbFractal(x, y);

			float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
			height = glm::clamp(height, 0.f, 1.f);

			float mask = glm::distance(0.5f*float(image->width), float(y)) / (0.5f*float(image->width));
			mask = 1.f - glm::clamp(mask, 0.f, 1.f);
			mask = glm::smoothstep(0.2f, 0.5f, sqrtf(mask));
			height *= sqrtf(mask);

			x = i; y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float ridge = cellnoise.GetNoise(x, y) / max;

			height = glm::mix(height, ridge, ridge*glm::smoothstep(0.4f, 0.7f, height));

			image->data[index++] = height;
		}
	}
}

void heightmap_image(struct byteimage *image, long seed, float frequency, float perturb)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(frequency);
	noise.SetFractalOctaves(5);
	noise.SetGradientPerturbAmp(perturb);

	const glm::vec2 center = glm::vec2(0.5f*float(image->width), 0.5f*float(image->height));
	unsigned int index = 0;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float x = i; float y = j;
			noise.GradientPerturbFractal(x, y);

			float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
			height = glm::clamp(height, 0.f, 1.f);

			float mask = glm::distance(0.5f*float(image->width), float(y)) / (0.5f*float(image->width));
			mask = 1.f - glm::clamp(mask, 0.f, 1.f);
			mask = glm::smoothstep(0.2f, 0.5f, sqrtf(mask));
			height *= sqrtf(mask);

			image->data[index++] = 255.f * height;
		}
	}
}

void gradient_image(struct byteimage *image, long seed, float perturb)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(0.005);
	noise.SetGradientPerturbAmp(perturb);

	const float height = float(image->height);
	unsigned int index = 0;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float y = i; float x = j;
			noise.GradientPerturbFractal(x, y);
			float lattitude = 1.f - (y / height);
			image->data[index++] = 255.f * glm::clamp(lattitude, 0.f, 1.f);
		}
	}
}
