struct cell;
struct vertex;
struct edge;

struct edge {
	int index;
	const struct vertex *v0 = nullptr;
	const struct vertex *v1 = nullptr;
	const struct cell *c0 = nullptr;
	const struct cell *c1 = nullptr;
};

struct vertex {
	int index;
	glm::vec2 position;
	std::vector<const struct vertex*> adjacent;
	std::vector<const struct cell*> cells;
};

struct cell {
	int index;
	glm::vec2 center;
	std::vector<const struct cell*> neighbors;
	std::vector<const struct vertex*> vertices;
	std::vector<const struct edge*> edges;
};

class Voronoi {
public:
	std::vector<struct cell> cells;
	std::vector<struct vertex> vertices;
	std::vector<struct edge> edges;
public:
	void gen_diagram(std::vector<glm::vec2> &locations, glm::vec2 min, glm::vec2 max, bool relax);
};
