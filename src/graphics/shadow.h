struct depthmap {
	GLuint FBO;
	GLuint texture;
	GLsizei height;
	GLsizei width;
};

class Shadow {
public:
	const uint8_t CASCADE_COUNT = 4;
	const glm::mat4 scalebias = glm::mat4(
		glm::vec4(0.5f, 0.0f, 0.0f, 0.0f), 
		glm::vec4(0.0f, 0.5f, 0.0f, 0.0f), 
		glm::vec4(0.0f, 0.0f, 0.5f, 0.0f), 
		glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
	);
	glm::mat4 shadowspace[4];
	glm::vec4 splitdepth;
public:
	Shadow(size_t texture_size);
	void update(const Camera *cam, glm::vec3 lightpos);
	void enable(void) const;
	void binddepth(unsigned int section) const;
	void disable(void) const;
	void bindtextures(GLenum unit) const;

private:
	depthmap depth;
};
