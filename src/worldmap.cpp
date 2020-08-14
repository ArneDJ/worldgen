#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <list>
#include <queue>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "extern/INIReader.h"
#include "extern/poisson_disk_sampling.h"

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"
#include "worldmap.h"

enum TEMPERATURE { COLD, TEMPERATE, WARM };
enum VEGETATION { ARID, DRY, HUMID };

static struct worldparams import_noiseparams(const char *fpath);
static struct branch *insert(const struct corner *confluence);
static void delete_basin(struct basin *tree);
static void prune_branches(struct branch *root);
static void stream_postorder(struct basin *tree);
static enum TEMPERATURE pick_temperature(float warmth);
static enum VEGETATION pick_vegetation(float rainfall);
static enum BIOME pick_biome(enum RELIEF relief, enum TEMPERATURE temper, enum VEGETATION veg);

static const size_t DIM = 256;
static const float POISSON_DISK_RADIUS = 8.F;
static const int MIN_STREAM_ORDER = 4;
static const size_t TERRA_IMAGE_RES = 512;
static const size_t MIN_WATER_BODY = 1024;
static const size_t MIN_MOUNTAIN_BODY = 128;
static const char *WORLDGEN_INI_FPATH = "worldgen.ini";

// default values in case values from the ini file are invalid
static const struct worldparams DEFAULT_WORLD_PARAMETERS = {
	// heightmap
	.frequency = 0.001f,
	.perturbfreq = 0.001f,
	.perturbamp = 200.f,
	.octaves = 6,
	.lacunarity = 2.5f,
	// temperature
	.tempfreq = 0.005f,
	.tempperturb = 100.f,
	// rain
	.rainblur = 25.f,
	// relief
	.lowland = 0.48f,
	.upland = 0.58f,
	.highland = 0.66f,
};

Worldmap::Worldmap(long seed, struct rectangle area) 
{
	this->seed = seed;
	this->area = area;
	this->params = import_noiseparams(WORLDGEN_INI_FPATH);

	terra = form_terra(TERRA_IMAGE_RES, this->seed, this->params);

	gen_diagram(DIM*DIM);

	gen_relief(&terra.heightmap);

	gen_rivers();

	gen_biomes();
};

Worldmap::~Worldmap(void)
{
	delete_byteimage(&terra.heightmap);
	delete_byteimage(&terra.tempmap);
	delete_byteimage(&terra.rainmap);

	for (auto &bas : basins) {
		delete_basin(&bas);
	}
}

void Worldmap::gen_diagram(unsigned int maxcandidates)
{
	float radius = POISSON_DISK_RADIUS;
	auto mmin = std::array<float, 2>{{area.min.x, area.min.y}};
	auto mmax = std::array<float, 2>{{area.max.x, area.max.y}};

	std::vector<std::array<float, 2>> candidates = thinks::PoissonDiskSampling(radius, mmin, mmax, 30, seed);
	std::vector<glm::vec2> locations;
	for (const auto &point : candidates) {
		locations.push_back(glm::vec2(point[0], point[1]));
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
		std::vector<struct corner*> adjacent;
		for (const auto &neighbor : vertex.adjacent) {
			adjacent.push_back(&corners[neighbor->index]);
		}
		std::vector<struct tile*> touches;
		for (const auto &cell : vertex.cells) {
			touches.push_back(&tiles[cell->index]);
		}

		struct corner c = {
			.index = vertex.index,
			.position = vertex.position,
			.adjacent = adjacent,
			.touches = touches,
			.frontier = false,
			.coast = false,
			.river = false
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
		} else {
			borders[index].t0 = &tiles[edge.c1->index];
			borders[index].frontier = true;
			borders[index].c0->frontier = true;
			borders[index].c1->frontier = true;
		}
		if (edge.c1 != nullptr) {
			borders[index].t1 = &tiles[edge.c1->index];
		} else {
			borders[index].t1 = &tiles[edge.c0->index];
			borders[index].frontier = true;
			borders[index].c0->frontier = true;
			borders[index].c1->frontier = true;
		}
	}
}

void Worldmap::gen_biomes(void)
{
	const float scale_x = float(TERRA_IMAGE_RES) / area.max.x;
	const float scale_y = float(TERRA_IMAGE_RES) / area.max.y;
	for (struct tile &t : tiles) {
		float warmth = sample_byteimage(scale_x*t.center.x, scale_y*t.center.y, RED, &terra.tempmap);
		float rain = sample_byteimage(scale_x*t.center.x, scale_y*t.center.y, RED, &terra.rainmap);
		enum TEMPERATURE temper = pick_temperature(warmth);
		enum VEGETATION veg = pick_vegetation(rain);
		t.biome = pick_biome(t.relief, temper, veg);
		if (t.biome == DESERT && t.relief == LOWLAND && t.river == true) {
			t.biome = FLOODPLAIN;
		}
	}
}

void Worldmap::gen_relief(const struct byteimage *heightmap)
{
	const float scale_x = float(TERRA_IMAGE_RES) / area.max.x;
	const float scale_y = float(TERRA_IMAGE_RES) / area.max.y;
	for (struct tile &t : tiles) {
		float height = sample_byteimage(scale_x*t.center.x, scale_y*t.center.y, RED, heightmap);
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

	// find coastal tiles
	for (auto &b : borders) {
		// use XOR to determine if land is different
		b.coast = b.t0->land ^ b.t1->land;
		b.t0->coast = b.coast;
		b.t1->coast = b.coast;
		b.c0->coast = b.coast;
		b.c1->coast = b.coast;
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
			std::queue<const struct tile*> queue;
			umap[&root] = true;
			queue.push(&root);
			marked.push_back(&root);

			while (queue.empty() == false) {
				const struct tile *v = queue.front();
				queue.pop();

				for (const auto &neighbor : v->neighbors) {
					if (umap[neighbor] == false) {
						umap[neighbor] = true;
						if (neighbor->relief == target) {
							queue.push(neighbor);
							marked.push_back(&tiles[neighbor->index]);
						}
					}
				}
			}
		}

		if (marked.size() > 0 && marked.size() < minsize) {
			for (struct tile *t : marked) {
				t->relief = replacement;
				if (target == SEABED) {
					t->land = true;
				}
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
			std::queue<const struct tile*> queue;
			umap[&root] = true;
			queue.push(&root);
			marked.push_back(&root);

			while (queue.empty() == false) {
				const struct tile *v = queue.front();
				queue.pop();

				for (const auto &neighbor : v->neighbors) {
					if (neighbor->relief == SEABED) {
						foundwater = true;
						break;
					}
					if (umap[neighbor] == false) {
						umap[neighbor] = true;
						if (neighbor->relief == LOWLAND || neighbor->relief == UPLAND) {
							queue.push(neighbor);
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

void Worldmap::gen_rivers(void)
{
	// construct the drainage basin candidate graph
	// only land and coast corners not on the edge of the map can be candidates for the graph
	// TODO only add nodes that are above a minimum rainfall in the graph
	std::vector<const struct corner*> graph;
	for (auto &c : corners) {
		if (c.coast && c.frontier == false) {
			graph.push_back(&c);
			c.river = true;
		} else {
			bool land = true;
			for (const auto &t : c.touches) {
				if (t->relief == SEABED) {
					land = false;
					break;
				}
			}
			if (land && c.frontier == false) {
				graph.push_back(&c);
				c.river = true;
			}
		}
	}

	gen_drainage_basins(graph);

	trim_river_basins();

	// after trimming make sure river properties of corners are correct
	for (auto &c : corners) {
		c.river = false;
	}
	for (const auto &bas : basins) {
		if (bas.mouth != nullptr) {
			std::queue<struct branch*> queue;
			queue.push(bas.mouth);
			while (!queue.empty()) {
				struct branch *cur = queue.front();
				queue.pop();
				corners[cur->confluence->index].river = true;
				if (cur->right != nullptr) {
					queue.push(cur->right);
				}
				if (cur->left != nullptr) {
					queue.push(cur->left);
				}
			}
		}
	}
	for (auto &c : corners) {
		for (auto &t : c.touches) {
			t->river = c.river;
		}
	}
}

void Worldmap::gen_drainage_basins(std::vector<const struct corner*> &graph)
{
	struct meta {
		bool visited;
		int elevation;
		int score;
	};

	std::unordered_map<const struct corner*, struct meta> umap;
	for (auto node : graph) {
		int weight = 0;
		for (const auto &t : node->touches) {
			if (t->relief == UPLAND) {
				weight += 3;
			} else if (t->relief == HIGHLAND) {
				weight += 4;
			}
		}
		struct meta data = {
			.visited = false,
			.elevation = weight,
			.score = 0
		};
		umap[node] = data;
	}

	// breadth first search
	for (auto root : graph) {
		if (root->coast) {
			std::queue<const struct corner*> frontier;
			umap[root].visited = true;
			frontier.push(root);
			while (!frontier.empty()) {
				const struct corner *v = frontier.front();
				frontier.pop();
				struct meta &vdata = umap[v];
				int depth = vdata.score + vdata.elevation + 1;
				for (auto neighbor : v->adjacent) {
					if (neighbor->river == true && neighbor->coast == false) {
						struct meta &ndata = umap[neighbor];
						if (ndata.visited == false) {
							ndata.visited = true;
							ndata.score = depth;
							frontier.push(neighbor);
						} else if (ndata.score > depth && ndata.elevation >= vdata.elevation) {
							ndata.score = depth;
							frontier.push(neighbor);
						}
					}
				}
			}
		}
	}

	// create the drainage basin binary tree
	for (auto node : graph) {
		umap[node].visited = false;
	}
	for (auto root : graph) {
		if (root->coast) {
			umap[root].visited = true;
			struct basin basn;
			struct branch *mouth = insert(root);
			basn.mouth = mouth;
			std::queue<struct branch*> frontier;
			frontier.push(mouth);
			while (!frontier.empty()) {
				struct branch *fork = frontier.front();
				const struct corner *v = fork->confluence;
				frontier.pop();
				struct meta &vdata = umap[v];
				for (auto neighbor : v->adjacent) {
					struct meta &ndata = umap[neighbor];
					bool valid = ndata.visited == false && neighbor->coast == false;
					if (valid) {
						if (ndata.score > vdata.score && ndata.elevation >= vdata.elevation) {
							ndata.visited = true;
							struct branch *child = insert(neighbor);
							frontier.push(child);
							if (fork->left == nullptr) {
								fork->left = child;
							} else if (fork->right == nullptr) {
								fork->right = child;
							}
						}
					}
				}
			}
			basins.push_back(basn);
		}
	}
}

void Worldmap::trim_river_basins(void)
{
	// assign stream order numbers
	for (auto &bas : basins) {
		stream_postorder(&bas);
	}
	// prune binary tree branch if the stream order is too low
	for (auto it = basins.begin(); it != basins.end(); ) {
		struct basin &bas = *it;
		std::queue<struct branch*> queue;
		queue.push(bas.mouth);
		while (!queue.empty()) {
			struct branch *cur = queue.front();
			queue.pop();

			if (cur->right != nullptr) {
				if (cur->right->streamorder < MIN_STREAM_ORDER) {
					prune_branches(cur->right);
					cur->right = nullptr;
				} else {
					queue.push(cur->right);
				}
			}
			if (cur->left != nullptr) {
				if (cur->left->streamorder < MIN_STREAM_ORDER) {
					prune_branches(cur->left);
					cur->left = nullptr;
				} else {
					queue.push(cur->left);
				}
			}
		}
		if (bas.mouth->right == nullptr && bas.mouth->left == nullptr) {
			delete bas.mouth;
			bas.mouth = nullptr;
			it = basins.erase(it);
		} else {
			++it;
		}
	}
}

static struct branch *insert(const struct corner *confluence)
{
	struct branch *node = new branch;
	node->confluence = confluence;
	node->left = nullptr;
	node->right = nullptr;
	node->streamorder = 1;

	return node;
}

// Strahler stream order
// https://en.wikipedia.org/wiki/Strahler_number
static inline int strahler(const struct branch *node) 
{
	// if node has no children it is a leaf with stream order 1
	if (node->left == nullptr && node->right == nullptr) {
		return 1;
	}

	int left = (node->left != nullptr) ? node->left->streamorder : 0;
	int right = (node->right != nullptr) ? node->right->streamorder : 0;

	if (left == right) {
		return std::max(left, right) + 1;
	} else {
		return std::max(left, right);
	}
}

// Shreve stream order
// https://en.wikipedia.org/wiki/Stream_order#Shreve_stream_order
static inline int shreve(const struct branch *node) 
{
	// if node has no children it is a leaf with stream order 1
	if (node->left == nullptr && node->right == nullptr) {
		return 1;
	}

	int left = (node->left != nullptr) ? node->left->streamorder : 0;
	int right = (node->right != nullptr) ? node->right->streamorder : 0;

	return left + right;
}

// post order tree traversal
static void stream_postorder(struct basin *tree)
{
	std::list<struct branch*> stack;
	struct branch *prev = nullptr;
	stack.push_back(tree->mouth);

	while (!stack.empty()) {
		struct branch *current = stack.back();

		if (prev == nullptr || prev->left == current || prev->right == current) {
			if (current->left != nullptr) {
				stack.push_back(current->left);
			} else if (current->right != nullptr) {
				stack.push_back(current->right);
			} else {
				current->streamorder = strahler(current);
				stack.pop_back();
			}
		} else if (current->left == prev) {
			if (current->right != nullptr) {
				stack.push_back(current->right);
			} else {
				current->streamorder = strahler(current);
				stack.pop_back();
			}
		} else if (current->right == prev) {
			current->streamorder = strahler(current);
			stack.pop_back();
		}

		prev = current;
	}
}

static void prune_branches(struct branch *root)
{
	if (root == nullptr) { return; }

	std::queue<struct branch*> queue;
	queue.push(root);

	struct branch *front = nullptr;

	while (!queue.empty()) {
		front = queue.front();
		queue.pop();
		if (front->left) { queue.push(front->left); }
		if (front->right) { queue.push(front->right); }

		delete front;
	}
}

static void delete_basin(struct basin *tree)
{
	if (tree->mouth == nullptr) { return; }

	prune_branches(tree->mouth);

	tree->mouth = nullptr;
}

static enum TEMPERATURE pick_temperature(float warmth)
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

static enum VEGETATION pick_vegetation(float rainfall)
{
	enum VEGETATION vegetation = ARID;

	if (rainfall < 0.25f) {
		vegetation = ARID;
	} else {
		std::random_device rd;
		std::mt19937 gen(rd());
		float p = glm::smoothstep(0.2f, 0.7f, rainfall);
		std::bernoulli_distribution d(p);
		if (d(gen) == false) {
			vegetation = DRY;
		} else {
			vegetation = HUMID;
		}
	}

	return vegetation;
};

static enum BIOME pick_biome(enum RELIEF relief, enum TEMPERATURE temper, enum VEGETATION veg)
{
	if (relief == SEABED) { return SEA; } // pretty obvious

	// mountain biomes
	if (relief == HIGHLAND) {
		if (temper == WARM && veg == ARID) {
			return BADLANDS;
		} else {
			return GLACIER;
		}
	}

	if (temper == COLD) {
		switch (veg) {
		case ARID: return STEPPE;
		case DRY: return PINE_GRASSLAND;
		case HUMID: return PINE_FOREST;
		};
	} else if (temper == TEMPERATE) {
		switch (veg) {
		case ARID: return STEPPE;
		case DRY: return BROADLEAF_GRASSLAND;
		case HUMID: return BROADLEAF_FOREST;
		};
	} else if (temper == WARM) {
		switch (veg) {
		case ARID: return DESERT;
		case DRY: return SAVANNA;
		case HUMID: return SHRUBLAND;
		};
	}

	return GLACIER; // the impossible happened
}

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

	float perturbfreq = reader.GetReal("", "HEIGHT_PERTURB_FREQUENCY", -1.f);
	if (perturbfreq > 0.f) { params.perturbfreq = perturbfreq; }

	float perturbamp = reader.GetReal("", "HEIGHT_PERTURB_AMP", -1.f);
	if (perturbamp > 0.f) { params.perturbamp = perturbamp; }

	unsigned int octaves = reader.GetInteger("", "HEIGHT_FRACTAL_OCTAVES", 0);
	if (octaves > 0) { params.octaves = octaves; }

	float lacunarity = reader.GetReal("", "HEIGHT_FRACTAL_LACUNARITY", -1.f);
	if (lacunarity > 0.f) { params.lacunarity = lacunarity; }

	float tempfreq = reader.GetReal("", "TEMPERATURE_FREQUENCY", -1.f);
	if (tempfreq > 0.f) { params.tempfreq = tempfreq; }

	float tempperturb = reader.GetReal("", "TEMPERATURE_PERTURB", -1.f);
	if (tempperturb > 0.f) { params.tempperturb = tempperturb; }

	float rainblur = reader.GetReal("", "RAIN_AMP", -1.f);
	if (rainblur > 0.f) { params.rainblur = rainblur; }

	float lowland = reader.GetReal("", "ELEVATION_LOWLAND", -1.f);
	if (lowland > 0.f && lowland < 1.f) { params.lowland = lowland; }

	float upland = reader.GetReal("", "ELEVATION_UPLAND", -1.f);
	if (upland > 0.f && upland < 1.f) { params.upland = upland; }

	float highland = reader.GetReal("", "ELEVATION_HIGHLAND", -1.f);
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

