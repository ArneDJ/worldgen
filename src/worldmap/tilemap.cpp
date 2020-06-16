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
#include "voronoi.h"
#include "tilemap.h"

#define MAX_RIVER_SIZE 2000
#define SEA_LEVEL 0.43f
#define MOUNTAIN_LEVEL 0.69f
#define NSITES 256*256

static enum TEMPERATURE sample_temperature(float warmth)
{
	enum TEMPERATURE temperature = COLD;
	if (warmth > 0.75f) {
		temperature = WARM;
	} else if (warmth < 0.25f) {
		temperature = COLD;
	} else {
		temperature = TEMPERATE;
	}

	return temperature;
}

static enum RELIEF sample_relief(float height)
{
	enum RELIEF relief = WATER;

	if (height < SEA_LEVEL) {
		relief = WATER;
	}
	if (height > MOUNTAIN_LEVEL) {
		relief = MOUNTAIN;
	}
	if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
		std::random_device rd;
		std::mt19937 gen(rd());
		float p = glm::smoothstep(SEA_LEVEL, MOUNTAIN_LEVEL, height);
		std::bernoulli_distribution d(p);
		if (d(gen) == false) {
			relief = PLAIN;
		} else {
			relief = HILLS;
		}
	}

	return relief;
}

static enum VEGETATION sample_vegetation(float rainfall) 
{
	enum VEGETATION vegetation = ARID;
	if (rainfall < 0.25f) {
		vegetation = ARID;
	} else {
		std::random_device rd;
		std::mt19937 gen(rd());
		float p = glm::smoothstep(0.2f, 1.f, rainfall);
		std::bernoulli_distribution d(sqrtf(p));
		if (d(gen) == false) {
			vegetation = DRY;
		} else {
			vegetation = HUMID;
		}
	}

	return vegetation;
};

static enum BIOME generate_biome(enum RELIEF relief, enum TEMPERATURE temperature, enum VEGETATION vegetation) 
{
	if (relief == WATER) { return OCEAN; }

	if (relief == MOUNTAIN) {
		if (temperature == WARM) {
			return BADLANDS; 
		} else {
			return ALPINE;
		}
	}

	enum BIOME biome = STEPPE;
	if (temperature == COLD) {
		switch (vegetation) {
		case ARID: biome = STEPPE; break;
		case DRY: biome = GRASSLAND; break;
		case HUMID: biome = TAIGA; break;
		};
	} else if (temperature == TEMPERATE) {
		switch (vegetation) {
		case ARID: biome = STEPPE; break;
		case DRY: biome = GRASSLAND; break;
		case HUMID: biome = FOREST; break;
		};
	} else if (temperature == WARM) {
		switch (vegetation) {
		case ARID: biome = DESERT; break;
		case DRY: biome = SAVANNA; break;
		case HUMID: biome = SAVANNA; break;
		};
	}

	return biome;
};

void Tilemap::gen_tiles(size_t max, const struct floatimage *heightimage, const struct byteimage *rainimage, const struct byteimage *temperatureimage)
{
	// generate random points in real space, not image space
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, max);

	const float ratio = float(heightimage->width) / float(max);

	// generate set of points based on elevation and rainfall
	std::vector<glm::vec2> points;

	for (int i = 0; i < NSITES; i++) {
		glm::vec2 point = glm::vec2(dis(gen), dis(gen));
		int x = int(point.x);
		int y = int(point.y);
		float height = sample_floatimage(ratio*x, ratio*y, 0, heightimage);
		float rain = sample_byteimage(ratio*x, ratio*y, 0, rainimage);
		float p = 1.f;
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			p = 1.f;
		} else if (height < SEA_LEVEL) {
			p = 0.05f;
		} else {
			p = 0.25f;
		}
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			p = glm::mix(p, rain, 0.75f);
		}
		std::bernoulli_distribution d(p);
		if (d(gen) == true) {
			points.push_back(point);
		}
	}

	voronoi.gen_diagram(points, max, max);
	tiles.resize(voronoi.cells.size());
	corners.resize(voronoi.vertices.size());

	// copy cell structure to tiles
	for (const auto &cell : voronoi.cells) {
		int x = int(cell.center.x);
		int y = int(cell.center.y);
		float height = sample_floatimage(ratio*x, ratio*y, 0, heightimage);
		float warmth = sample_byteimage(ratio*x, ratio*y, 0, temperatureimage);
		float rain = sample_byteimage(ratio*x, ratio*y, 0, rainimage);
		enum RELIEF relief = sample_relief(height);
		enum TEMPERATURE temperature = sample_temperature(warmth);
		enum VEGETATION vegetation = sample_vegetation(rain);
		enum BIOME biome = generate_biome(relief, temperature, vegetation);
		std::vector<const struct tile*> neighbors;
		for (const auto &neighbor : cell.neighbors) {
			neighbors.push_back(&tiles[neighbor->index]);
		}
		std::vector<const struct corner*> corn;
		for (const auto &vertex : cell.vertices) {
			corn.push_back(&corners[vertex->index]);
		}
		struct tile t = {
			.index = cell.index,
			.relief = relief,
			.temperature = temperature,
			.vegetation = vegetation,
			.biome = biome,
			.coast = false,
			.river = false,
			.site = &cell,
			.neighbors = neighbors,
			.corners = corn,
		};

		tiles[cell.index] = t;
	}

	// copy vertex structure to corners
	for (const auto &vertex : voronoi.vertices) {
		std::vector<struct corner*> neighbors;
		for (const auto &neighbor : vertex.adjacent) {
			neighbors.push_back(&corners[neighbor->index]);
		}
		std::vector<const struct tile*> tils;
		for (const auto &cell : vertex.cells) {
			tils.push_back(&tiles[cell->index]);
		}
		struct corner c = {
			.index = vertex.index,
			.coastal = false,
			.river = false,
			.border = false,
			.v = &vertex,
			.neighbors = neighbors,
			.tiles = tils,
		};

		corners[vertex.index] = c;
	}

	// find coastal tiles
	for (auto &t : tiles) {
		bool sea = false;
		bool land = false;
		for (auto neighbor : t.neighbors) {
			if (neighbor->relief == WATER) {
				sea = true;
			}
			if (neighbor->relief != WATER) {
				land = true;
			}
		}

		if (sea == true && land == true ) {
			t.coast = true;
		}

		// turn lakes of only 1 tile into marshes
		if (t.relief == WATER && sea == false) {
			t.relief = PLAIN;
			t.biome = MARSH;
		}

		// assign ocean and sea tiles
		if (t.relief == WATER) {
			if (t.coast == true) {
				t.biome = SEA;
			} else {
				t.biome = OCEAN;
			}
		}
	}

	// find coastal and border corners
	for (auto &c : corners) {
		bool sea = false;
		bool land = false;
		for (const auto &tile : c.tiles) {
			if (tile->relief == WATER) {
				sea = true;
			}
			if (tile->relief != WATER) {
				land = true;
			}
		}
		if (sea == true && land == true ) {
			c.coastal = true;
		}

		auto ncells = c.tiles.size();
		if (ncells < 3) {
			c.border = true;
		}
	}

	// generate rivers
	std::uniform_int_distribution<size_t> distr(0, corners.size() - 1);
	for (int i = 0; i < 500; i++) {
		struct river river;
		size_t start = distr(gen);
		struct corner *source = &corners[start];
		bool rejected = true;
		float height = sample_floatimage(ratio*source->v->position.x, ratio*source->v->position.y, 0, heightimage);
		// river sources may only start above a certain height
		if (height > 0.5f) {
			river.source = source;
			struct corner *start = source;
			struct corner *previous = source;
			size_t river_size = 0;
			while (start->coastal == false && river_size < MAX_RIVER_SIZE) {
				float min = 1.f;
				struct corner *next = nullptr;
				for (auto &neighbor : start->neighbors) {
					if (neighbor != previous) {
						float neighborheight = sample_floatimage(ratio*neighbor->v->position.x, ratio*neighbor->v->position.y, 0, heightimage);
						if (neighborheight < min) { 
							next = neighbor;
							min = neighborheight;
						}
					}
				}

				// if we still don't find a valid corner reject the river
				if (next == nullptr) {
					rejected = true;
					break;
				}

				// don't let rivers flow at the edge of the map
				if (next->border == true) {
					rejected = true;
					break;
				}

				glm::vec2 direction = glm::vec2(next->v->position.x, next->v->position.y) - glm::vec2(start->v->position.x, start->v->position.y);
				struct border segment = {
					.a = start,
					.b = next,
					.direction = direction,
				};

				river.segments.push_back(segment);

				if (next->coastal == true) {
					rejected = false;
					river.mouth = next;
				}

				previous = start;
				start = next;
				river_size++;
			}
		}

		if (rejected == false) {
			rivers.push_back(river);
		}
	}

	// validate rivers
	for (const auto &river : rivers) {
		for (const auto &segment : river.segments) {
			corners[segment.a->index].river = true;
			corners[segment.b->index].river = true;
		}
	}

	// assign floodplains and marshes 
	for (auto &tile : tiles) {
		size_t river_count = 0;
		for (const auto &corner : tile.corners) {
			if (corner->river == true) {
				river_count++;
				if (tile.biome == DESERT) {
					tile.biome = FLOODPLAIN;
				}
			}
		}
		if (river_count > 5 && tile.relief == PLAIN) {
			tile.biome = MARSH;
		}
	}
}

