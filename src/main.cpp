#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
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
#include "worldmap.h"

static const struct rectangle MAP_AREA = { 
	.min = {0.f, 0.f}, 
	.max = {2048.f, 2048.f}
};

int main(int argc, char *argv[])
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<long> seedgen;
	//long seed = seedgen(gen);
	long seed = 1337;

	Worldmap worldmap = {seed, MAP_AREA};

	struct byteimage image = blank_byteimage(1, 2048, 2048);
	unsigned char red[] = {255, 0, 0};
	unsigned char blu[] = {0, 0, 255};
	for (const auto &t : worldmap.tiles) {
		unsigned char color[3];
		float base = 0.25f;
		if (t.land) {
			base = 1.f;
		}

		color[0] = 255 * base;
		color[1] = 255 * base;
		color[2] = 255 * base;
		for (const auto &b : t.borders) {
			draw_triangle(t.center, b->c1->position, b->c0->position, image.data, image.width, image.height, image.nchannels, color);
		}
		//plot(c.center.x, c.center.y, image.data, image.width, image.height, image.nchannels, red);
	}

	stbi_write_png("second.png", image.width, image.height, image.nchannels, image.data, image.width);

	return 0;
}
