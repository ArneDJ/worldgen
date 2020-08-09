#include <iostream>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb_image_write.h"
#include "extern/FastNoise.h"
#include "extern/INIReader.h"

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"

static const size_t DIM = 256;

// default values in case values from the ini file are invalid
static const struct worldparams DEFAULT_WORLD_PARAMETERS = {
	.frequency = 0.001f,
	.perturbfreq = 0.001f,
	.perturbamp = 200.f,
	.octaves = 6,
	.lacunarity = 2.5f,
	.tempfreq = 0.005f,
	.tempperturb = 100.f,
	.rainperturb = 1.f,
	.tempinfluence = 0.6f,
	.rainblur = 50.f,
	.lowland = 0.48f,
	.upland = 0.58f,
	.highland = 0.66f,
};

struct worldparams import_noiseparams(const char *fpath)
{
	INIReader reader = {fpath};

	// init with default values
	struct worldparams params = DEFAULT_WORLD_PARAMETERS;

	if (reader.ParseError() != 0) {
		perror(fpath);
		return params;
	}

	float frequency = reader.GetReal("", "HEIGHT_FREQUENCY", -1.f);
	if (params.frequency > 0.f) { params.frequency = frequency; }

	float perturbfreq = reader.GetReal("", "PERTURB_FREQUENCY", -1.f);
	if (perturbfreq > 0.f) { params.perturbfreq = perturbfreq; }

	float perturbamp = reader.GetReal("", "PERTURB_AMP", -1.f);
	if (perturbamp > 0.f) { params.perturbamp = perturbamp; }

	unsigned int octaves = reader.GetInteger("", "FRACTAL_OCTAVES", -1);
	if (octaves > 0) { params.octaves = octaves; }

	float lacunarity = reader.GetReal("", "FRACTAL_LACUNARITY", -1.f);
	if (lacunarity > 0.f) { params.lacunarity = lacunarity; }

	float tempfreq = reader.GetReal("", "TEMPERATURE_FREQUENCY", -1.f);
	if (tempfreq > 0.f) { params.tempfreq = tempfreq; }

	float tempperturb = reader.GetReal("", "TEMPERATURE_PERTURB", -1.f);
	if (tempperturb > 0.f) { params.tempperturb = tempperturb; }

	float rainperturb = reader.GetReal("", "RAIN_PERTURB", -1.f);
	if (rainperturb > 0.f) { params.rainperturb = rainperturb; }

	float tempinfluence = reader.GetReal("", "TEMPERATURE_INFLUENCE", -1.f);
	if (tempinfluence > 0.f) { params.tempinfluence = tempinfluence; }

	float rainblur = reader.GetReal("", "RAIN_AMP", -1.f);
	if (rainblur > 0.f) { params.rainblur = rainblur; }

	float lowland = reader.GetReal("", "LOWLAND_ELEVATION", -1.f);
	if (lowland > 0.f) { params.lowland = lowland; }

	float upland = reader.GetReal("", "UPLAND_ELEVATION", -1.f);
	if (upland > 0.f) { params.upland = upland; }

	float highland = reader.GetReal("", "HIGHLAND_ELEVATION", -1.f);
	if (highland > 0.f) { params.highland = highland; }

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

	std::uniform_int_distribution<long> seedgen;
	long seed = seedgen(gen);

	Terraform terra = {SIDELEN, seed, params};

	stbi_write_png("elevation.png", terra.heightmap.width, terra.heightmap.height, terra.heightmap.nchannels, terra.heightmap.data, terra.heightmap.width);
	stbi_write_png("temperature.png", terra.tempmap.width, terra.tempmap.height, terra.tempmap.nchannels, terra.tempmap.data, terra.tempmap.width);
	stbi_write_png("rain.png", terra.rainmap.width, terra.rainmap.height, terra.rainmap.nchannels, terra.rainmap.data, terra.rainmap.width);

	return 0;
}
