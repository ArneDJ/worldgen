struct vcell {
	glm::vec2 center;
	std::vector<vcell*> neighbors;
	std::vector<glm::vec4> borders;
};

void do_voronoi(struct byteimage *image, const struct byteimage *heightimage);

void gen_cells(struct byteimage *image);
