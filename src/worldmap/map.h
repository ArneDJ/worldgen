enum RELIEF_TYPE {
	WATER,
	PLAIN,
	HILLS,
	MOUNTAIN,
};

enum TEMPERATURE {
	COLD,
	TEMPERATE,
	WARM,
};

enum VEGETATION {
	ARID,
	DRY,
	HUMID,
};

enum BIOME {
	OCEAN,
	SEA,
	MARSH,
	FOREST,
	GRASSLAND,
	TAIGA,
	SAVANNA,
	STEPPE,
	DESERT,
	BADLANDS,
	ALPINE,
	FLOODPLAIN,
};

struct tile;

struct corner {
	int index;
	bool coastal;
	bool river;
	const struct vertex *v;
	std::vector<struct corner*> neighbors;
	std::vector<const struct tile*> tiles;
};

struct border {
	const struct corner *a;
	const struct corner *b;
	glm::vec2 direction;
};

struct river {
	std::vector<struct border> segments;
	const struct corner *source;
	const struct corner *mouth;
};

struct tile {
	int index;
	enum RELIEF_TYPE relief;
	enum TEMPERATURE temperature;
	enum VEGETATION vegetation;
	enum BIOME biome;
	bool coast;
	bool river;
	const struct cell *site;
	std::vector<const struct tile*> neighbors;
	std::vector<const struct corner*> corners;
};

