#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <queue>
#include <list>
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
	unsigned char blu[] = {0, 0, 255};
	unsigned char wit[] = {255, 255, 255};
	unsigned char ora[] = {255, 0, 0};
	unsigned char pur[] = {255, 0, 255};
	glm::vec3 red = {1.f, 0.f, 0.f};
	glm::vec3 sea = {0.2f, 0.5f, 0.95f};
	glm::vec3 grassland = {0.2f, 1.f, 0.2f};
	glm::vec3 desert = {1.f, 1.f, 0.2f};
	glm::vec3 taiga = {0.2f, 0.95f, 0.6f};
	glm::vec3 glacier = {0.8f, 0.8f, 1.f};
	glm::vec3 forest = 0.8f * grassland;
	glm::vec3 taiga_forest = 0.8f * taiga;
	glm::vec3 steppe = glm::mix(grassland, desert, 0.5f);
	glm::vec3 shrubland = glm::mix(forest, desert, 0.75f);
	glm::vec3 savanna = glm::mix(grassland, desert, 0.75f);
	glm::vec3 badlands = glm::mix(red, desert, 0.75f);
	glm::vec3 floodplain = glm::mix(forest, desert, 0.5f);

	for (const auto &t : worldmap->tiles) {
		unsigned char color[3];
		glm::vec3 rgb = {1.f, 1.f, 1.f};
		float base = 0.25f;
		switch (t.relief) {
		case SEABED : base = 0.75f; break;
		case LOWLAND : base = 0.8f; break;
		case UPLAND : base = 0.9f; break;
		case HIGHLAND : base = 1.f; break;
		};
		switch (t.biome) {
		case SEA: rgb = sea; break;
		case BROADLEAF_FOREST: rgb = forest; break;
		case PINE_FOREST: rgb = taiga_forest; break;
		case PINE_GRASSLAND: rgb = taiga; break;
		case SAVANNA: rgb = savanna; break;
		case STEPPE: rgb = steppe; break;
		case DESERT: rgb = desert; break;
		case GLACIER: rgb = glacier; break;
		case SHRUBLAND: rgb = shrubland; break;
		case BROADLEAF_GRASSLAND: rgb = grassland; break;
		case FLOODPLAIN: rgb = floodplain; break;
		case BADLANDS: rgb = badlands; break;
		};

		color[0] = 255 * base * rgb.x;
		color[1] = 255 * base * rgb.y;
		color[2] = 255 * base * rgb.z;

		glm::vec2 a = {round(t.center.x), round(t.center.y)};
		for (const auto &bord : t.borders) {
			// round points to rasterize properly
			glm::vec2 b = {round(bord->c0->position.x), round(bord->c0->position.y)};
			glm::vec2 c = {round(bord->c1->position.x), round(bord->c1->position.y)};
			draw_triangle(a, b, c, image.data, image.width, image.height, image.nchannels, color);
		}
		if (t.site == TOWN) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x+1, a.y+1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x+1, a.y-1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x-1, a.y+1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x-1, a.y-1, image.data, image.width, image.height, image.nchannels, pur);
		} else if (t.site == CASTLE) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x+1, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x, a.y+1, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x-1, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x, a.y-1, image.data, image.width, image.height, image.nchannels, blu);
		} else if (t.site == VILLAGE) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, ora);
		}
	}

	for (const auto &b : worldmap->borders) {
		if (b.river) {
			draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, blu);
		} else {
			//draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, ora);
		}
	}

	stbi_flip_vertically_on_write(true);
	stbi_write_png("world.png", image.width, image.height, image.nchannels, image.data, image.width*image.nchannels);
	//stbi_write_png("rain.png", worldmap->terra.rainmap.width, worldmap->terra.rainmap.height, worldmap->terra.rainmap.nchannels, worldmap->terra.rainmap.data, worldmap->terra.rainmap.width*worldmap->terra.rainmap.nchannels);

	delete_byteimage(&image);
}

void print_cultures(const Worldmap *worldmap)
{
	struct byteimage image = blank_byteimage(3, 2048, 2048);
	unsigned char color[] = {255, 255, 255};
	unsigned char blu[] = {0, 0, 255};
	unsigned char wit[] = {255, 255, 255};
	unsigned char ora[] = {255, 0, 0};
	unsigned char pur[] = {255, 0, 255};
	glm::vec3 red = {1.f, 0.f, 0.f};
	glm::vec3 sea = {0.2f, 0.5f, 0.95f};
	glm::vec3 grassland = {0.2f, 1.f, 0.2f};
	glm::vec3 desert = {1.f, 1.f, 0.2f};
	glm::vec3 taiga = {0.2f, 0.95f, 0.6f};
	glm::vec3 glacier = {0.8f, 0.8f, 1.f};
	glm::vec3 forest = 0.8f * grassland;
	glm::vec3 taiga_forest = 0.8f * taiga;
	glm::vec3 steppe = glm::mix(grassland, desert, 0.5f);
	glm::vec3 shrubland = glm::mix(forest, desert, 0.75f);
	glm::vec3 savanna = glm::mix(grassland, desert, 0.75f);
	glm::vec3 badlands = glm::mix(red, desert, 0.75f);
	glm::vec3 floodplain = glm::mix(forest, desert, 0.5f);

	for (const auto &t : worldmap->tiles) {
		unsigned char color[3];
		glm::vec3 rgb = {1.f, 1.f, 1.f};
		float base = 0.25f;
		switch (t.relief) {
		case SEABED : base = 0.75f; break;
		case LOWLAND : base = 0.8f; break;
		case UPLAND : base = 0.9f; break;
		case HIGHLAND : base = 1.f; break;
		};
		switch (t.biome) {
		case SEA: rgb = sea; break;
		case BROADLEAF_FOREST: rgb = forest; break;
		case PINE_FOREST: rgb = taiga_forest; break;
		case PINE_GRASSLAND: rgb = taiga; break;
		case SAVANNA: rgb = savanna; break;
		case STEPPE: rgb = steppe; break;
		case DESERT: rgb = desert; break;
		case GLACIER: rgb = glacier; break;
		case SHRUBLAND: rgb = shrubland; break;
		case BROADLEAF_GRASSLAND: rgb = grassland; break;
		case FLOODPLAIN: rgb = floodplain; break;
		case BADLANDS: rgb = badlands; break;
		};

		color[0] = 255 * base * rgb.x;
		color[1] = 255 * base * rgb.y;
		color[2] = 255 * base * rgb.z;

		glm::vec2 a = {round(t.center.x), round(t.center.y)};
		for (const auto &bord : t.borders) {
			// round points to rasterize properly
			glm::vec2 b = {round(bord->c0->position.x), round(bord->c0->position.y)};
			glm::vec2 c = {round(bord->c1->position.x), round(bord->c1->position.y)};
			draw_triangle(a, b, c, image.data, image.width, image.height, image.nchannels, color);
		}
	}

	std::random_device rd;
	std::mt19937 gen(worldmap->seed);
	for (const auto &hold : worldmap->holdings) {
		std::uniform_real_distribution<float> distrib(0.f, 1.f);
		color[0] = distrib(gen) * 255;
		color[1] = distrib(gen) * 255;
		color[2] = distrib(gen) * 255;
		for (const auto &land : hold.lands) {
			glm::vec2 a = {round(land->center.x), round(land->center.y)};
			for (const auto &bord : land->borders) {
				// round points to rasterize properly
				glm::vec2 b = {round(bord->c0->position.x), round(bord->c0->position.y)};
				glm::vec2 c = {round(bord->c1->position.x), round(bord->c1->position.y)};
				draw_triangle(a, b, c, image.data, image.width, image.height, image.nchannels, color);
			}
		}
	}
	for (const auto &t : worldmap->tiles) {
		glm::vec2 a = {round(t.center.x), round(t.center.y)};
		if (t.site == TOWN) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x+1, a.y+1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x+1, a.y-1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x-1, a.y+1, image.data, image.width, image.height, image.nchannels, pur);
			plot(a.x-1, a.y-1, image.data, image.width, image.height, image.nchannels, pur);
		} else if (t.site == CASTLE) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x+1, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x, a.y+1, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x-1, a.y, image.data, image.width, image.height, image.nchannels, blu);
			plot(a.x, a.y-1, image.data, image.width, image.height, image.nchannels, blu);
		} else if (t.site == VILLAGE) {
			plot(a.x, a.y, image.data, image.width, image.height, image.nchannels, ora);
		}
	}
	for (auto &bord : worldmap->borders) {
		if (bord.t0->hold != bord.t1->hold) {
			glm::vec2 b = {round(bord.c0->position.x), round(bord.c0->position.y)};
			glm::vec2 c = {round(bord.c1->position.x), round(bord.c1->position.y)};
			draw_line(b.x, b.y, c.x, c.y, image.data, image.width, image.height, image.nchannels, ora);
		}
	}

	stbi_flip_vertically_on_write(true);
	stbi_write_png("holdings.png", image.width, image.height, image.nchannels, image.data, image.width*image.nchannels);

	delete_byteimage(&image);
}

void print_hold(const struct holding *hold)
{
	printf("The name of the hold is %s\n", hold->name.c_str());
	if (hold->center->site == TOWN) {
		printf("Its capital is the town of %s located at %f, %f\n", hold->center->name.c_str(), hold->center->center.x, hold->center->center.y);
	}
	if (hold->center->site == CASTLE) {
		printf("Its capital is the castle of %s located at %f, %f\n", hold->center->name.c_str(), hold->center->center.x, hold->center->center.y);
	}
	printf("The villages of %s are\n", hold->name.c_str());
	for (const auto fief : hold->lands) {
		if (fief->site == VILLAGE) {
			printf("%s\n", fief->name.c_str());
		}
	}
	printf("The neighboring holds are\n");
	for (const auto neighbor : hold->neighbors) {
		printf("%s\n", neighbor->name.c_str());
	}
}

int main(int argc, char *argv[])
{
	printf("Name thy world: ");
	std::string name;
	std::cin >> name;
	long seed = std::hash<std::string>()(name);

	if (name == "1337") {
		seed = 1337;
	}

	Worldmap worldmap = {seed, MAP_AREA};

	print_hold(&worldmap.holdings.front());
	print_image(&worldmap);
	print_cultures(&worldmap);

	return 0;
}
