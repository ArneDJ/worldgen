#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <queue>
#include <list>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb_image_write.h"
#include "extern/FastNoise.h"
#include "extern/INIReader.h"

#include "extern/CDT.h"

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"
#include "worldmap.h"

static const struct rectangle MAP_AREA = { 
	.min = {0.f, 0.f}, 
	.max = {2048.f, 2048.f}
};

static void fill_image_tiles(const Worldmap *worldmap, int start, int end, struct byteimage *image)
{
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

	for (int i = start; i < end; i++) {
		const auto &t = worldmap->tiles[i];
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
			draw_triangle(a, b, c, image->data, image->width, image->height, image->nchannels, color);
		}
	}
}

void print_image(const Worldmap *worldmap)
{
	unsigned char blu[] = {0, 0, 255};
	unsigned char red[] = {255, 0, 0};
	struct byteimage image = blank_byteimage(3, 2048, 2048);

	auto start = std::chrono::steady_clock::now();
	//fill_image_tiles(worldmap, 0, worldmap->tiles.size(), &image);
	std::vector<std::thread*> threads;
	int step = worldmap->tiles.size() / 8;
	for (int i = 0; i < worldmap->tiles.size(); i += step) {
		int end = i + step;
		if (end > worldmap->tiles.size()) { end = worldmap->tiles.size(); }
		std::thread *thread = new std::thread(fill_image_tiles, worldmap, i, end, &image);
		threads.push_back(thread);
	}
	for (int i = 0; i < threads.size(); i++) {
		threads[i]->join();
		delete threads[i];
	}
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	std::cout << "image elapsed time: " << elapsed_seconds.count() << "s\n";

	for (const auto &b : worldmap->borders) {
		if (b.river) {
			draw_thick_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, 2, image.data, image.width, image.height, image.nchannels, blu);
		} else {
			//draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, red);
		}
	}
	/*
	for (const auto &c : worldmap->corners) {
		if (c.river) {
			draw_filled_circle(c.position.x, c.position.y, 0.5*c.depth, image.data, image.width, image.height, image.nchannels, red);
		}
	}
	*/

	stbi_flip_vertically_on_write(true);
	stbi_write_png("world.png", image.width, image.height, image.nchannels, image.data, image.width*image.nchannels);

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

	std::vector<std::thread*> threads;
	int step = worldmap->tiles.size() / 8;
	for (int i = 0; i < worldmap->tiles.size(); i += step) {
		int end = i + step;
		if (end > worldmap->tiles.size()) { end = worldmap->tiles.size(); }
		std::thread *thread = new std::thread(fill_image_tiles, worldmap, i, end, &image);
		threads.push_back(thread);
	}
	for (int i = 0; i < threads.size(); i++) {
		threads[i]->join();
		delete threads[i];
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

// use BFS to find landmasses
// find the vertices that define borders (rivers, water, mountains)
// add borders as constrain edges
// triangulate using CDT library

// TODO checkout seed: sdsd
int main(int argc, char *argv[])
{
	struct CustomEdge {
		std::pair<size_t, size_t> vertices;
	};

	printf("Name thy world: ");
	std::string name;
	std::cin >> name;
	long seed = std::hash<std::string>()(name);

	if (name == "1337") {
		seed = 1337;
	}

	auto start = std::chrono::steady_clock::now();
	Worldmap worldmap = {seed, MAP_AREA};
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	std::cout << "worldgen time: " << elapsed_seconds.count() << "s\n";

	print_hold(&worldmap.holdings.front());
	print_image(&worldmap);
	print_cultures(&worldmap);

	// triangulate worldmap
	struct byteimage image = blank_byteimage(3, 2048, 2048);
	std::random_device rd;
	std::mt19937 gen(44);

	using Triangulation = CDT::Triangulation<float>;
	Triangulation cdt = Triangulation(CDT::FindingClosestPoint::ClosestRandom, 10);

	unsigned char red[] = {255, 0, 0};
	std::vector<glm::vec2> points;
	std::unordered_map<uint32_t, size_t> umap;
	std::unordered_map<uint32_t, bool> marked;
	size_t index = 0;
	for (const auto &c : worldmap.corners) {
		bool notfrontier = false;
		for (const auto &adj : c.adjacent) {
			if (adj->wall == false && adj->frontier == true) { notfrontier = true; }
		}
		marked[c.index] = false;
		if (c.coast == true) {
			points.push_back(c.position);
			umap[c.index] = index++;
			marked[c.index] = true;
		} else if (c.frontier == true) {
			bool land = false;
			for (const auto &t : c.touches) {
				if (t->land) { land = true; }
			}
			if (land == true && c.wall == false) {
				points.push_back(c.position);
				umap[c.index] = index++;
				marked[c.index] = true;
			}
		} else if (c.wall == true) {
			points.push_back(c.position);
			umap[c.index] = index++;
			marked[c.index] = true;
		}
		if (marked[c.index] == false && c.frontier == true && c.wall == true && notfrontier == true) {
			points.push_back(c.position);
			umap[c.index] = index++;
			marked[c.index] = true;
		}
	}

	std::vector<CustomEdge> edges;
	for (const auto &b : worldmap.borders) {
		size_t left = umap[b.c0->index];
		size_t right = umap[b.c1->index];
		if (marked[b.c0->index] == true && marked[b.c1->index] == true) {
		if (b.coast == true || b.wall == true) {
			struct CustomEdge edge;
			edge.vertices = std::make_pair(left, right);
			edges.push_back(edge);
			draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, red);
		} else if (b.frontier == true) {
			if (b.t0->land == true && b.t1->land == true) {
				struct CustomEdge edge;
				edge.vertices = std::make_pair(left, right);
				edges.push_back(edge);
				draw_line(b.c0->position.x, b.c0->position.y, b.c1->position.x, b.c1->position.y, image.data, image.width, image.height, image.nchannels, red);
			}
		}
		}
	}

	cdt.insertVertices(
		points.begin(),
		points.end(),
		[](const glm::vec2& p){ return p[0]; },
		[](const glm::vec2& p){ return p[1]; }
	);
	cdt.insertEdges(
		edges.begin(),
		edges.end(),
		[](const CustomEdge& e){ return e.vertices.first; },
		[](const CustomEdge& e){ return e.vertices.second; }
	);
	//cdt.eraseSuperTriangle();
	cdt.eraseOuterTrianglesAndHoles();
	//cdt.eraseOuterTriangles();


	unsigned char color[3];

	for (const auto &triangle : cdt.triangles) {
	std::uniform_real_distribution<float> distrib(0.5f, 1.f);
	color[0] = distrib(gen) * 255;
	color[1] = distrib(gen) * 255;
	color[2] = distrib(gen) * 255;
	glm::vec2 vertices[3];
	for (int i = 0; i < 3; i++) {
	//printf("%d\n", triangle.vertices[i]);
	size_t index = triangle.vertices[i];
	const auto pos = cdt.vertices[index].pos;
	vertices[i].x = pos.x;
	vertices[i].y = pos.y;
	//printf("%f, %f\n", pos.x, pos.y);
	}
	draw_triangle(vertices[0], vertices[1], vertices[2], image.data, image.width, image.height, image.nchannels, color);
	}

	stbi_flip_vertically_on_write(true);
	stbi_write_png("triangles.png", image.width, image.height, image.nchannels, image.data, image.width*image.nchannels);

	delete_byteimage(&image);


	return 0;
}
