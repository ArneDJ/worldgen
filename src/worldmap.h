struct tile;
struct corner;
struct border;

enum RELIEF {
	SEABED,
	LOWLAND,
	UPLAND,
	HIGHLAND
};

enum BIOME {
	SEA,
	GLACIER,
	DESERT,
	SAVANNA,
	SHRUBLAND,
	STEPPE,
	BROADLEAF_GRASSLAND,
	BROADLEAF_FOREST,
	PINE_GRASSLAND,
	PINE_FOREST,
	FLOODPLAIN
};

struct border {
	const struct corner *c0 = nullptr;
	const struct corner *c1 = nullptr;
	const struct tile *t0 = nullptr;
	const struct tile *t1 = nullptr;
};

struct corner {
	int index;
	glm::vec2 position;
	std::vector<const struct corner*> adjacent;
	std::vector<const struct tile*> touches;
};

struct tile {
	// graph data
	int index;
	glm::vec2 center;
	std::vector<const struct tile*> neighbors;
	std::vector<const struct corner*> corners;
	std::vector<const struct border*> borders;
	// world data
	bool land;
	enum SURFACE surface;
	enum BIOME biome;
};
