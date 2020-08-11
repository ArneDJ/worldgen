#include <iostream>
#include <string>
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

void print_image(const Worldmap *worldmap)
{
	struct byteimage image = blank_byteimage(3, 2048, 2048);
	unsigned char red[] = {255, 0, 0};
	unsigned char blu[] = {0, 0, 255};
	unsigned char wit[] = {255, 255, 255};
	for (const auto &t : worldmap->tiles) {
		unsigned char color[3];
		float base = 0.25f;
		switch (t.relief) {
		case SEABED : base = 0.25f; break;
		case LOWLAND : base = 0.5f; break;
		case UPLAND : base = 0.6f; break;
		case HIGHLAND : base = 1.f; break;
		};

		color[0] = 255 * base;
		color[1] = 255 * base;
		color[2] = 255 * base;
		for (const auto &b : t.borders) {
			draw_triangle(t.center, b->c1->position, b->c0->position, image.data, image.width, image.height, image.nchannels, color);
		}
	}
	/*
	for (const auto &b : worldmap->borders) {
		if (b.river) {
			draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, blu);
		}
	}
	*/
	for (const auto &b : worldmap->basins) {
		for (const auto &chan : b.channels) {
			if (chan.right != nullptr) {
				draw_line(chan.confluence->position.x, chan.confluence->position.y, chan.right->position.x, chan.right->position.y, image.data, image.width, image.height, image.nchannels, blu);
			}
			if (chan.left != nullptr) {
				draw_line(chan.confluence->position.x, chan.confluence->position.y, chan.left->position.x, chan.left->position.y, image.data, image.width, image.height, image.nchannels, blu);
			}
		}
	}
	for (const auto &c : worldmap->corners) {
		if (c.coast) {
			plot(c.position.x, c.position.y, image.data, image.width, image.height, image.nchannels, red);
		}
		if (c.river) {
			plot(c.position.x, c.position.y, image.data, image.width, image.height, image.nchannels, wit);
		}
	}

	stbi_write_png("output.png", image.width, image.height, image.nchannels, image.data, image.width*image.nchannels);

	delete_byteimage(&image);
}

int main(int argc, char *argv[])
{
	//printf("Name thy world: ");
	//std::string name;
	//std::cin >> name;

	long seed = 1337;
	//long seed = std::hash<std::string>()(name); // I copy pasted this from the internet so I have no idea how it works

	Worldmap worldmap = {seed, MAP_AREA};

	print_image(&worldmap);

	return 0;
}
