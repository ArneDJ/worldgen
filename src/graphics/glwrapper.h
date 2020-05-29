#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct mesh {
	GLuint VAO, VBO, EBO;
	GLenum mode; // rendering mode
	GLsizei ecount; // element count
	bool indexed;
};

struct TBO {
	GLuint texture;
	GLuint buffer;
};

struct mesh gen_patch_grid(const size_t sidelength, const float offset);

struct mesh gen_mapcube(void);

/*
 * a -- b
 * |	|
 * |	|
 * c -- d
 */
struct mesh gen_quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

struct mesh gen_cardinal_quads(void);

struct mesh create_cube(void);

void delete_mesh(const struct mesh *m);

GLuint bind_texture(const struct floatimage *image, GLenum internalformat, GLenum format, GLenum type);

GLuint bind_mipmap_texture(struct byteimage *image, GLenum internalformat, GLenum format, GLenum type);

void activate_texture(GLenum unit, GLenum target, GLuint texture); 

GLuint load_TGA_cubemap(const char *fpath[6]);

void instance_static_VAO(GLuint VAO, std::vector<glm::mat4> *transforms);

GLuint instance_dynamic_VAO(GLuint VAO, size_t instancecount);

struct TBO create_TBO(GLsizeiptr size, GLenum internalformat);
