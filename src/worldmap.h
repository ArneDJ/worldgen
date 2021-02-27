struct tile;
struct corner;
struct border;

enum SITE { VACANT, VILLAGE, TOWN, CASTLE, RUIN };

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
	FLOODPLAIN,
	BADLANDS
};

struct border {
	int index;
	struct corner *c0 = nullptr;
	struct corner *c1 = nullptr;
	struct tile *t0 = nullptr;
	struct tile *t1 = nullptr;
	// world data
	bool frontier;
	bool coast;
	bool river;
	bool wall;
};

struct corner {
	// graph data
	int index;
	glm::vec2 position;
	std::vector<struct corner*> adjacent;
	std::vector<struct tile*> touches;
	// world data
	bool frontier;
	bool coast;
	bool river;
	bool wall;
	int depth;
};

struct tile {
	// graph data
	int index;
	glm::vec2 center;
	std::vector<const struct tile*> neighbors;
	std::vector<const struct corner*> corners;
	std::vector<const struct border*> borders;
	// world data
	bool frontier;
	bool land;
	bool coast;
	bool river;
	enum RELIEF relief;
	enum BIOME biome;
	enum SITE site;
	std::string name;
	struct holding *hold = nullptr;
};

struct branch {
	const struct corner *confluence = nullptr;
	struct branch *left = nullptr;
	struct branch *right = nullptr;
	int streamorder;
	int depth;
};

struct basin {
	struct branch *mouth; // binary tree root
	size_t height; // binary tree height
};

struct holding {
	int index;
	std::string name;
	struct tile *center; // center tile of the hold that contains a fortification
	std::vector<struct tile*> lands; // tiles that the holding consists of
	std::vector<const struct holding*> neighbors; // neighbouring holds
};

class Worldmap {
public:
	struct terraform terra;
	std::vector<struct tile> tiles;
	std::vector<struct corner> corners;
	std::vector<struct border> borders;
	std::list<struct basin> basins;
	std::list<struct holding> holdings;
	long seed;
public:
	Worldmap(long seed, struct rectangle area);
	~Worldmap(void);
private:
	struct worldparams params;
	struct rectangle area;
private:
	void gen_diagram(unsigned int maxcandidates);
	void gen_relief(const struct byteimage *heightmap);
	void gen_rivers(void);
	void gen_biomes(void);
	void gen_sites(void);
	void gen_holds(void);
	void name_holds(void);
	void name_sites(void);
	void floodfill_relief(unsigned int minsize, enum RELIEF target, enum RELIEF replacement);
	void remove_echoriads(void);
	void gen_drainage_basins(std::vector<const struct corner*> &graph);
	void trim_river_basins(void);
	void correct_border_rivers(void);
	void erode_mountains(void);
	void correct_walls(void);
	void trim_stubby_rivers(void);
};
