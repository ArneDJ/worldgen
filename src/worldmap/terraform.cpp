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

#include "../graphics/imp.h"
#include "terraform.h"

struct floatimage gen_heightmap(size_t width, size_t height, long seed, float freq)
{
	struct floatimage image = {
		.data = new float[height*width],
		.nchannels = 1,
		.width = width,
		.height = height,
	};
	simplex_image(&image, seed, freq, 200.f);

	return image;
}

struct byteimage gen_temperature(long seed)
{
	const size_t size = 2048;

	struct byteimage image = {
		.data = new unsigned char[size*size],
		.nchannels = 1,
		.width = size,
		.height = size,
	};

	gradient_image(&image, seed, 100.f);

	return image;
}

struct byteimage gen_rainfall(const struct floatimage *heightimage, const struct byteimage *tempimage, long seed)
{
	const size_t size = heightimage->width * heightimage->height * heightimage->nchannels;

	struct byteimage image = {
		.data = new unsigned char[size],
		.nchannels = heightimage->nchannels,
		.width = heightimage->width,
		.height = heightimage->height,
	};

	//simplex_image(&image, seed, 0.002f, 200.f);

	for (auto i = 0; i < size; i++) {
			float height = heightimage->data[i];
			height = height < 0.5 ? 0.f : 1.f;
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

Terraform::Terraform(size_t width, size_t height, long seed, float freq)
{
	heightmap = gen_heightmap(width, height, seed, freq);		
	temperature = gen_temperature(seed);
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
