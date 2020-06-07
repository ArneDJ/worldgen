struct cell;

struct corner {
	int index;
	glm::vec2 position;
	std::vector<struct corner*> adjacent;
	std::vector<const struct cell*> cells;
};

struct cell {
	int index;
	glm::vec2 center;
	std::vector<glm::vec4> borders;
	std::vector<const struct cell*> neighbors;
	std::vector<const struct corner*> corners;
};

class Voronoi {
public:
	std::vector<struct cell> cells;
	std::vector<struct corner> corners;
public:
	Voronoi(std::vector<glm::vec2> &locations, size_t width, size_t height)
	{
		gen_diagram(locations, width, height);
	}
private:
	void gen_diagram(std::vector<glm::vec2> &locations, size_t width, size_t height);
};
