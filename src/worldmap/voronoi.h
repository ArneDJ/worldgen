struct cell;

struct vertex {
	int index;
	glm::vec2 position;
	std::vector<struct vertex*> adjacent;
	std::vector<const struct cell*> cells;
};

struct cell {
	int index;
	glm::vec2 center;
	std::vector<glm::vec4> borders;
	std::vector<const struct cell*> neighbors;
	std::vector<const struct vertex*> vertices;
};

class Voronoi {
public:
	std::vector<struct cell> cells;
	std::vector<struct vertex> vertices;
public:
	void gen_diagram(std::vector<glm::vec2> &locations, size_t width, size_t height);
};
