#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <list>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "extern/INIReader.h"

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"
#include "worldmap.h"

static const size_t TERRA_IMAGE_RES = 512;
static const size_t MIN_WATER_BODY = 1024;
static const size_t MIN_MOUNTAIN_BODY = 128;
static const char *WORLDGEN_INI_FPATH = "worldgen.ini";

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

static struct worldparams import_noiseparams(const char *fpath)
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

	unsigned int octaves = reader.GetInteger("", "FRACTAL_OCTAVES", 0);
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
	if (lowland > 0.f && lowland < 1.f) { params.lowland = lowland; }

	float upland = reader.GetReal("", "UPLAND_ELEVATION", -1.f);
	if (upland > 0.f && upland < 1.f) { params.upland = upland; }

	float highland = reader.GetReal("", "HIGHLAND_ELEVATION", -1.f);
	if (highland > 0.f && highland < 1.f) { params.highland = highland; }

	// lowland can't be higher than upland
	// upland can't be higher than highland
	std::array<float, 3> s = {params.lowland, params.upland, params.highland};
	std::sort(s.begin(), s.end());
	params.lowland = s[0];
	params.upland = s[1];
	params.highland = s[2];

	return params;
}

Worldmap::Worldmap(long seed, struct rectangle area) 
{
	this->seed = seed;
	this->area = area;
	this->params = import_noiseparams(WORLDGEN_INI_FPATH);

	Terraform terra = {TERRA_IMAGE_RES, this->seed, this->params};

	gen_diagram(255*255);

	const float scale_x = float(TERRA_IMAGE_RES) / this->area.max.x;
	const float scale_y = float(TERRA_IMAGE_RES) / this->area.max.y;
	for (struct tile &t : tiles) {
		float height = sample_byteimage(scale_x*t.center.x, scale_y*t.center.y, RED, &terra.heightmap);
		t.land = (height < params.lowland) ? false : true;
		if (height < params.lowland) { 
			t.relief = SEABED;
		} else if (height < params.upland) {
			t.relief = LOWLAND;
		} else if (height < params.highland) {
			t.relief = UPLAND;
		} else {
			t.relief = HIGHLAND;
		}
	}

	floodfill_relief(MIN_WATER_BODY, SEABED, LOWLAND);
	floodfill_relief(MIN_MOUNTAIN_BODY, HIGHLAND, UPLAND);

	remove_echoriads();
};

void Worldmap::gen_diagram(unsigned int maxcandidates)
{
	// generate random points
	std::random_device rd;
	std::mt19937 gen(seed);
	std::uniform_real_distribution<float> dist_x(area.min.x, area.max.x);
	std::uniform_real_distribution<float> dist_y(area.min.y, area.max.y);

	std::vector<glm::vec2> locations;
	for (unsigned int i = 0; i < maxcandidates; i++) {
		locations.push_back((glm::vec2) {dist_x(gen), dist_y(gen)});
	}

	// copy voronoi graph
	Voronoi voronoi;
	voronoi.gen_diagram(locations, area.min, area.max, true);

	tiles.resize(voronoi.cells.size());
	corners.resize(voronoi.vertices.size());
	borders.resize(voronoi.edges.size());

	// adopt cell structures
	for (const auto &cell : voronoi.cells) {
		std::vector<const struct tile*> tneighbors;
		for (const auto &neighbor : cell.neighbors) {
			tneighbors.push_back(&tiles[neighbor->index]);
		}
		std::vector<const struct corner*> tcorners;
		for (const auto &vertex : cell.vertices) {
			tcorners.push_back(&corners[vertex->index]);
		}
		std::vector<const struct border*> tborders;
		for (const auto &edge : cell.edges) {
			tborders.push_back(&borders[edge->index]);
		}

		struct tile t = {
			.index = cell.index,
			.center = cell.center,
			.neighbors = tneighbors,
			.corners = tcorners,
			.borders = tborders,
		};

		tiles[cell.index] = t;
	}

	// adapt vertex structures
	for (const auto &vertex : voronoi.vertices) {
		std::vector<const struct corner*> adjacent;
		for (const auto &neighbor : vertex.adjacent) {
			adjacent.push_back(&corners[neighbor->index]);
		}
		std::vector<const struct tile*> touches;
		for (const auto &cell : vertex.cells) {
			touches.push_back(&tiles[cell->index]);
		}

		bool border = (touches.size() < 3) ? true : false;

		struct corner c = {
			.index = vertex.index,
			.position = vertex.position,
			.adjacent = adjacent,
			.touches = touches,
			.border = border,
		};

		corners[vertex.index] = c;
	}

	// adapt edge structures
	for (const auto &edge : voronoi.edges) {
		int index = edge.index;
		borders[index].c0 = &corners[edge.v0->index];
		borders[index].c1 = &corners[edge.v1->index];
		if (edge.c0 != nullptr) {
			borders[index].t0 = &tiles[edge.c0->index];
		}
		if (edge.c1 != nullptr) {
			borders[index].t1 = &tiles[edge.c1->index];
		}
	}
}

void Worldmap::floodfill_relief(unsigned int minsize, enum RELIEF target, enum RELIEF replacement)
{
	std::unordered_map<const struct tile*, bool> umap;
	for (struct tile &t : tiles) {
		umap[&t] = false;
	}

	for (struct tile &root : tiles) {
		std::vector<struct tile*> marked;
		if (umap[&root] == false && root.relief == target) {
			std::list<const struct tile*> queue;
			umap[&root] = true;
			queue.push_back(&root);
			marked.push_back(&root);

			while (queue.empty() == false) {
				const struct tile *v = queue.front();
				queue.pop_front();

				for (const auto &neighbor : v->neighbors) {
					if (umap[neighbor] == false) {
						umap[neighbor] = true;
						if (neighbor->relief == target) {
							queue.push_back(neighbor);
							marked.push_back(&tiles[neighbor->index]);
						}
					}
				}
			}
		}

		if (marked.size() > 0 && marked.size() < minsize) {
			for (struct tile *t : marked) {
				t->relief = replacement;
			}
		}
	}
}

// removes encircling mountains from the worldmap
// uses a slightly different version of the floodfill algorithm
void Worldmap::remove_echoriads(void)
{
	std::unordered_map<const struct tile*, bool> umap;
	for (struct tile &t : tiles) {
		umap[&t] = false;
	}

	for (struct tile &root : tiles) {
		bool foundwater = false;
		std::vector<struct tile*> marked;
		bool target = (root.relief == LOWLAND) || (root.relief == UPLAND);
		if (umap[&root] == false && target == true) {
			std::list<const struct tile*> queue;
			umap[&root] = true;
			queue.push_back(&root);
			marked.push_back(&root);

			while (queue.empty() == false) {
				const struct tile *v = queue.front();
				queue.pop_front();

				for (const auto &neighbor : v->neighbors) {
					if (neighbor->relief == SEABED) {
						foundwater = true;
						break;
					}
					if (umap[neighbor] == false) {
						umap[neighbor] = true;
						if (neighbor->relief == LOWLAND || neighbor->relief == UPLAND) {
							queue.push_back(neighbor);
							marked.push_back(&tiles[neighbor->index]);
						}
					}
				}
			}
		}

		if (marked.size() > 0 && foundwater == false) {
			for (struct tile *t : marked) {
				t->relief = HIGHLAND;
			}
		}
	}
}
