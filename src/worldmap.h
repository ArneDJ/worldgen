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
	int index;
	const struct corner *c0 = nullptr;
	const struct corner *c1 = nullptr;
	const struct tile *t0 = nullptr;
	const struct tile *t1 = nullptr;
};

struct corner {
	// graph data
	int index;
	glm::vec2 position;
	std::vector<const struct corner*> adjacent;
	std::vector<const struct tile*> touches;
	// world data
	bool border;
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
	enum RELIEF relief;
	enum BIOME biome;
};

class Worldmap {
public:
	std::vector<struct tile> tiles;
	std::vector<struct corner> corners;
	std::vector<struct border> borders;
	struct rectangle area;
	long seed;
public:
	Worldmap(long seed, struct rectangle area);
private:
	struct worldparams params;
private:
	void gen_diagram(unsigned int maxcandidates);
	void floodfill_relief(unsigned int minsize, enum RELIEF target, enum RELIEF replacement);
	void remove_echoriads(void);
};
