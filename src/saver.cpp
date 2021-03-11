
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
// for doing the actual serialization
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

#include "geom.h"
#include "imp.h"
#include "voronoi.h"
#include "terra.h"
#include "worldmap.h"
#include "saver.h"

struct tile_record {
	// graph data
	uint32_t index;
	// world data
	bool frontier;
	bool land;
	bool coast;
	bool river;
	// graph data
	float center_x;
	float center_y;
	std::vector<uint32_t> neighbors;
	std::vector<uint32_t> corners;
	std::vector<uint32_t> borders;
	//
	//float amp;
	uint8_t relief;
	uint8_t biome;
	uint8_t site;
	//std::string name;
	int32_t holding = -1;

	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(index), CEREAL_NVP(frontier), CEREAL_NVP(land), CEREAL_NVP(coast), CEREAL_NVP(river), CEREAL_NVP(center_x), CEREAL_NVP(center_y), CEREAL_NVP(neighbors), CEREAL_NVP(corners), CEREAL_NVP(borders), CEREAL_NVP(relief), CEREAL_NVP(biome), CEREAL_NVP(site), CEREAL_NVP(holding));
	}
};

struct corner_record {
	// graph data
	uint32_t index;
	float position_x;
	float position_y;
	std::vector<uint32_t> adjacent;
	std::vector<uint32_t> touches;
	// world data
	bool frontier;
	bool coast;
	bool river;
	bool wall;
	int depth;
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(index), CEREAL_NVP(position_x), CEREAL_NVP(position_y), CEREAL_NVP(adjacent), CEREAL_NVP(touches), CEREAL_NVP(frontier), CEREAL_NVP(coast), CEREAL_NVP(river), CEREAL_NVP(wall), CEREAL_NVP(depth));
	}
};

struct border_record {
	uint32_t index;
	uint32_t c0;
	uint32_t c1;
	uint32_t t0;
	uint32_t t1;
	// world data
	bool frontier;
	bool coast;
	bool river;
	bool wall;
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(index), CEREAL_NVP(c0), CEREAL_NVP(c1), CEREAL_NVP(t0), CEREAL_NVP(t1), CEREAL_NVP(frontier), CEREAL_NVP(coast), CEREAL_NVP(river), CEREAL_NVP(wall));
	}
};
	
void WorldSerializer::save(const Worldmap *world, const std::string &filepath)
{
	uint32_t tilecount = world->tiles.size();
	uint32_t cornercount = world->corners.size();
	uint32_t bordercount = world->borders.size();
	std::vector<struct tile_record> tile_records;
	std::vector<struct corner_record> corner_records;
	std::vector<struct border_record> border_records;

	// the tiles
	for (const auto &til : world->tiles) {
		struct tile_record record;
		record.index = til.index;
		record.frontier = til.frontier;
		record.land = til.land;
		record.coast = til.coast;
		record.center_x = til.center.x;
		record.center_y = til.center.y;
		for (const auto &neighbor : til.neighbors) {
			record.neighbors.push_back(neighbor->index);
		}
		for (const auto &corner : til.corners) {
			record.corners.push_back(corner->index);
		}
		for (const auto &border : til.borders) {
			record.borders.push_back(border->index);
		}
		//record.amp = til.amp;
		record.relief = uint8_t(til.relief);
		record.biome = uint8_t(til.biome);
		record.site = uint8_t(til.site);
		if (til.hold) {
			record.holding = til.hold->index;
		} else {
			record.holding = -1;
		}
		//
		tile_records.push_back(record);
	}

	// the corners
	for (const auto &corn : world->corners) {
		struct corner_record record;
		record.index = corn.index;
		record.position_x = corn.position.x;
		record.position_y = corn.position.y;
		for (const auto &adj : corn.adjacent) {
			record.adjacent.push_back(adj->index);
		}
		for (const auto &til : corn.touches) {
			record.touches.push_back(til->index);
		}
		// world data
		record.frontier = corn.frontier;
		record.coast = corn.coast;
		record.river = corn.river;
		record.wall = corn.wall;
		record.depth = corn.depth;
		//
		corner_records.push_back(record);
	}

	// the borders
	for (const auto &bord : world->borders) {
		struct border_record record;
		record.index = bord.index;
		record.c0 = bord.c0->index;
		record.c1 = bord.c1->index;
		record.t0 = bord.t0->index;
		record.t1 = bord.t1->index;
		// world data
		record.frontier = bord.frontier;
		record.coast = bord.coast;
		record.river = bord.river;
		record.wall = bord.wall;
		//
		border_records.push_back(record);
	}

	//std::ofstream os(filepath);
	//cereal::JSONOutputArchive archive(os);
	std::ofstream os(filepath, std::ios::binary);
	cereal::BinaryOutputArchive archive(os);

	archive(
		cereal::make_nvp("seed", world->seed), 
		cereal::make_nvp("tilecount", tilecount), 
		cereal::make_nvp("cornercount", cornercount), 
		cereal::make_nvp("bordercount", bordercount), 
		cereal::make_nvp("tiles", tile_records),
		cereal::make_nvp("corners", corner_records),
		cereal::make_nvp("corners", border_records)
	);
}
	
void WorldSerializer::load(const std::string &filepath)
{
	tiles.clear();
	corners.clear();
	borders.clear();

	uint32_t tilecount = 0;
	uint32_t cornercount = 0;
	uint32_t bordercount = 0;
	std::vector<struct tile_record> tile_records;
	std::vector<struct corner_record> corner_records;
	std::vector<struct border_record> border_records;

	std::ifstream is(filepath, std::ios::binary);
	cereal::BinaryInputArchive archive(is);

	archive(
		cereal::make_nvp("seed", seed), 
		cereal::make_nvp("tilecount", tilecount), 
		cereal::make_nvp("cornercount", cornercount), 
		cereal::make_nvp("bordercount", bordercount), 
		cereal::make_nvp("tiles", tile_records),
		cereal::make_nvp("corners", corner_records),
		cereal::make_nvp("corners", border_records)
	);

	tiles.resize(tilecount);
	corners.resize(cornercount);
	borders.resize(bordercount);

	// the tiles
	for (const auto &record : tile_records) {
		struct tile til;
		til.index = record.index;
		til.frontier = record.frontier;
		til.land = record.land;
		til.coast = record.coast;
		til.center.x = record.center_x;
		til.center.y = record.center_y;
		for (const auto &neighbor : record.neighbors) {
			til.neighbors.push_back(&tiles[neighbor]);
		}
		for (const auto &corner : record.corners) {
			til.corners.push_back(&corners[corner]);
		}
		for (const auto &border : record.borders) {
			til.borders.push_back(&borders[border]);
		}
		//record.amp = til.amp;
		til.relief = static_cast<enum RELIEF>(record.relief);
		til.biome = static_cast<enum BIOME>(record.biome);
		til.site = static_cast<enum SITE>(record.site);
		til.hold = nullptr;
		//
		tiles[til.index] = til;
	}

	// the corners
	for (const auto &record : corner_records) {
		struct corner corn;
		corn.index = record.index;
		corn.position.x = record.position_x;
		corn.position.y = record.position_y;
		for (const auto &adj : record.adjacent) {
			corn.adjacent.push_back(&corners[adj]);
		}
		for (const auto &index : record.touches) {
			corn.touches.push_back(&tiles[index]);
		}
		// world data
		corn.frontier = record.frontier;
		corn.coast = record.coast;
		corn.river = record.river;
		corn.wall = record.wall;
		corn.depth = record.depth;
		//
		corners[corn.index] = corn;
	}

	// the borders
	for (const auto &record : border_records) {
		struct border bord;
		bord.index = record.index;
		bord.c0 = &corners[record.c0];
		bord.c1 = &corners[record.c1];
		bord.t0 = &tiles[record.t0];
		bord.t1 = &tiles[record.t1];
		// world data
		bord.frontier = record.frontier;
		bord.coast = record.coast;
		bord.river = record.river;
		bord.wall = record.wall;
		//
		borders[bord.index] = bord;
	}

	std::cout << seed << std::endl;
}
