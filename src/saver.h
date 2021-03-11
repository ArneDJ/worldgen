
class WorldSerializer {
public:
	std::vector<struct tile> tiles;
	std::vector<struct corner> corners;
	std::vector<struct border> borders;
	//std::list<struct basin> basins;
	//std::list<struct holding> holdings;
	long seed;
public:
	void load(const std::string &filepath);
	void save(const Worldmap *world, const std::string &filepath);
};
