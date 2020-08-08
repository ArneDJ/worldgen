#include <iostream>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "FastNoise.h"

#include "INIReader.h"

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"

// default values in case values from the ini file are invalid
static const size_t DIM = 256;
static const float LOWLAND_ELEVATION = 0.48f;
static const float UPLAND_ELEVATION = 0.58f;
static const float HIGHLAND_ELEVATION = 0.66f;
static const float HEIGHT_FREQUENCY = 0.001f;
static const float PERTURB_FREQUENCY = 0.001f;
static const float PERTURB_AMP = 200.f;
static const unsigned int FRACTAL_OCTAVES = 6;
static const float FRACTAL_LACUNARITY = 2.5f;

struct worldparams import_noiseparams(const char *fpath)
{
	INIReader reader = {fpath};

	struct worldparams params = {
		.frequency = HEIGHT_FREQUENCY,
		.perturbfreq = PERTURB_FREQUENCY,
		.perturbamp = PERTURB_AMP,
		.octaves = FRACTAL_OCTAVES,
		.lacunarity = FRACTAL_LACUNARITY,
		.tempfreq = 0.005f,
		.tempperturb = 100.f,
		.rainperturb = 1.f,
		.lowland = LOWLAND_ELEVATION,
		.upland = UPLAND_ELEVATION,
		.highland = HIGHLAND_ELEVATION,
	};

	if (reader.ParseError() != 0) {
		perror(fpath);
		return params;
	}

	params.frequency = reader.GetReal("", "HEIGHT_FREQUENCY", -1.f);
	if (params.frequency <= 0.f) { params.frequency = HEIGHT_FREQUENCY; }

	params.perturbfreq = reader.GetReal("", "PERTURB_FREQUENCY", -1.f);
	if (params.perturbfreq <= 0.f) { params.perturbfreq = PERTURB_FREQUENCY; }

	params.perturbamp = reader.GetReal("", "PERTURB_AMP", -1.f);
	if (params.perturbamp <= 0.f) { params.perturbamp = PERTURB_AMP; }

	params.octaves = reader.GetInteger("", "FRACTAL_OCTAVES", -1);
	if (params.octaves <= 0) { params.octaves = FRACTAL_OCTAVES; }

	params.lacunarity = reader.GetReal("", "FRACTAL_LACUNARITY", -1.f);
	if (params.lacunarity <= 0.f) { params.lacunarity = FRACTAL_LACUNARITY; }

	params.lowland = reader.GetReal("", "LOWLAND_ELEVATION", -1.f);
	if (params.lowland <= 0.f) { params.lowland = LOWLAND_ELEVATION; }

	params.upland = reader.GetReal("", "UPLAND_ELEVATION", -1.f);
	if (params.upland <= 0.f) { params.upland = UPLAND_ELEVATION; }

	params.highland = reader.GetReal("", "HIGHLAND_ELEVATION", -1.f);
	if (params.highland <= 0.f) { params.highland = HIGHLAND_ELEVATION; }

	return params;
}

int main(int argc, char *argv[])
{
	size_t dim = DIM;

	struct worldparams params = import_noiseparams("worldgen.ini");

	const size_t SIDELEN = 2048;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distrib(0.f, SIDELEN);

	std::vector<glm::vec2> locations;
	for (int i = 0; i < dim*dim; i++) {
		locations.push_back((glm::vec2) {distrib(gen), distrib(gen)});
	}

	Voronoi voronoi;
	voronoi.gen_diagram(locations, (glm::vec2) {0.f, 0.f}, (glm::vec2) {SIDELEN, SIDELEN}, true);

	struct byteimage image = {
		.data = new unsigned char[SIDELEN*SIDELEN],
		.nchannels = 1,
		.width = SIDELEN,
		.height = SIDELEN
	};
	memset(image.data, 0, image.nchannels*image.width*image.height);

	std::uniform_int_distribution<long> seedgen;
	long seed = seedgen(gen);

	Terraform terra = {SIDELEN, seed, params};
	for (int i = 0; i < SIDELEN*SIDELEN; i++) {
		image.data[i] = terra.heightmap.data[i] * 255;
	}

	stbi_write_png("rain.png", terra.rainmap.width, terra.rainmap.height, terra.rainmap.nchannels, terra.rainmap.data, terra.rainmap.width);
	stbi_write_png("elevation.png", image.width, image.height, image.nchannels, image.data, image.width);

	delete_byteimage(&image);

	return 0;
}
