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

// default values in case values from the ini file are invalid
static const size_t DIM = 256;
static const float LOWLAND_ELEVATION = 0.48f;
static const float UPLAND_ELEVATION = 0.58f;
static const float HIGHLAND_ELEVATION = 0.66f;
static const float HEIGHT_FREQUENCY = 0.001f;
static const float PERTURB_FREQUENCY = 0.001f;
static const float PERTURB_AMP = 200.f;
static const size_t FRACTAL_OCTAVES = 6;
static const float FRACTAL_LACUNARITY = 2.5f;

int main(int argc, char *argv[])
{
	size_t dim = DIM;
	float lowland = LOWLAND_ELEVATION;
	float upland = UPLAND_ELEVATION;
	float highland = HIGHLAND_ELEVATION;
	float freq = HEIGHT_FREQUENCY;
	float perturbfreq = PERTURB_FREQUENCY;
	float perturbamp = PERTURB_AMP;
	size_t fractoctaves = FRACTAL_OCTAVES;
	float fractlacun = FRACTAL_LACUNARITY;
	INIReader reader = {"world.ini"};

	if (reader.ParseError() != 0) {
		std::cout << "Can't load 'world.ini'\n";
		return 1;
	}

	dim = reader.GetInteger("", "DIM", -1);
	if (dim <= 0) {
		dim = DIM;
	}
	lowland = reader.GetReal("", "LOWLAND_ELEVATION", -1.f);
	if (lowland <= 0.f) {
		lowland = LOWLAND_ELEVATION;
	}
	upland = reader.GetReal("", "UPLAND_ELEVATION", -1.f);
	if (upland <= 0.f) {
		upland = UPLAND_ELEVATION;
	}
	highland = reader.GetReal("", "HIGHLAND_ELEVATION", -1.f);
	if (highland <= 0.f) {
		highland = HIGHLAND_ELEVATION;
	}
	freq = reader.GetReal("", "HEIGHT_FREQUENCY", -1.f);
	if (freq <= 0.f) {
		freq = HEIGHT_FREQUENCY;
	}
	perturbfreq = reader.GetReal("", "PERTURB_FREQUENCY", -1.f);
	if (perturbfreq <= 0.f) {
		perturbfreq = PERTURB_FREQUENCY;
	}
	perturbamp = reader.GetReal("", "PERTURB_AMP", -1.f);
	if (perturbamp <= 0.f) {
		perturbamp = PERTURB_AMP;
	}
	fractoctaves = reader.GetInteger("", "FRACTAL_OCTAVES", -1);
	if (fractoctaves <= 0) {
		fractoctaves = FRACTAL_OCTAVES;
	}
	fractlacun = reader.GetReal("", "FRACTAL_LACUNARITY", -1.f);
	if (fractlacun <= 0.f) {
		fractlacun = FRACTAL_LACUNARITY;
	}

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
		.data = new unsigned char[SIDELEN*SIDELEN*3],
		.nchannels = 3,
		.width = SIDELEN,
		.height = SIDELEN
	};
	memset(image.data, 0, image.nchannels*image.width*image.height);

	std::uniform_int_distribution<long> seedgen;
	FastNoise noise;
	noise.SetSeed(seedgen(gen));
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(freq);
	noise.SetPerturbFrequency(perturbfreq);
	noise.SetFractalOctaves(fractoctaves);
	noise.SetFractalLacunarity(fractlacun);
	noise.SetGradientPerturbAmp(perturbamp);

	unsigned char red[] = {255, 0, 0};
	unsigned char blu[] = {0, 0, 255};
	for (const auto &c : voronoi.cells) {
		unsigned char color[3];
		float base = 1.f;
		float x = c.center.x;
		float y = c.center.y;
   		noise.GradientPerturbFractal(x, y);
   		float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
		if (height < lowland) {
			base = 0.25f;
		} else if (height < upland) {
			base = 0.5f;
		} else if (height < highland) {
			base = 0.55f;
		}

		color[0] = 255 * base;
		color[1] = 255 * base;
		color[2] = 255 * base;
		for (const auto &e : c.edges) {
			draw_triangle(c.center, e->v1->position, e->v0->position, image.data, image.width, image.height, image.nchannels, color);
		}
		plot(c.center.x, c.center.y, image.data, image.width, image.height, image.nchannels, red);
	}

	stbi_write_png("output.png", image.width, image.height, image.nchannels, image.data, image.width*3);


	delete_byteimage(&image);

	return 0;
}
