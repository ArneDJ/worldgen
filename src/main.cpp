#include <iostream>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "FastNoise.h"

#include "voronoi.h"
#include "imp.h"

int main(int argc, char *argv[])
{
	const size_t SIDELEN = 2048;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distrib(0.f, SIDELEN);

	std::vector<glm::vec2> locations;
	for (int i = 0; i < 10000; i++) {
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
	noise.SetFrequency(0.001f);
	noise.SetPerturbFrequency(0.001f);
	noise.SetFractalOctaves(6);
	noise.SetFractalLacunarity(2.5f);
	noise.SetGradientPerturbAmp(200.f);

	unsigned char red[] = {255, 0, 0};
	unsigned char blu[] = {0, 0, 255};
	for (const auto &c : voronoi.cells) {
		unsigned char color[3];
		float base = 1.f;
		float x = c.center.x;
		float y = c.center.y;
   		noise.GradientPerturbFractal(x, y);
   		float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
		if (height < 0.48f) {
			base = 0.25f;
		} else if (height < 0.58f) {
			base = 0.5f;
		} else if (height < 0.65f) {
			base = 0.6f;
		}

		color[0] = 255 * base;
		color[1] = 255 * base;
		color[2] = 255 * base;
		for (const auto &e : c.edges) {
			draw_triangle(c.center, e->v1->position, e->v0->position, image.data, image.width, image.height, image.nchannels, color);
		}
		for (const auto &e : c.edges) {
			draw_triangle(c.center, e->v0->position, e->v1->position, image.data, image.width, image.height, image.nchannels, color);
		}
		plot(c.center.x, c.center.y, image.data, image.width, image.height, image.nchannels, red);
	}

	stbi_write_png("output.png", image.width, image.height, image.nchannels, image.data, image.width*3);


	delete_byteimage(&image);

	return 0;
}
