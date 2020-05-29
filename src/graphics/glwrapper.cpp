#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stbimage/stb_image.h"

#include "imp.h"
#include "glwrapper.h"

// make a square patch grid along the x and z axis, needs a tessellation shader to render
struct mesh gen_patch_grid(const size_t sidelength, const float offset)
{
	struct mesh patch = {
		.VAO = 0, .VBO = 0, .EBO = 0,
		.mode = GL_PATCHES,
		.ecount = GLsizei(4 * sidelength * sidelength),
		.indexed = false
	};

	std::vector<glm::vec3> vertices;
	vertices.reserve(patch.ecount);

	glm::vec3 origin = glm::vec3(0.f, 0.f, 0.f);
	for (int x = 0; x < sidelength; x++) {
		for (int z = 0; z < sidelength; z++) {
			vertices.push_back(glm::vec3(origin.x, origin.y, origin.z));
			vertices.push_back(glm::vec3(origin.x+offset, origin.y, origin.z));
			vertices.push_back(glm::vec3(origin.x, origin.y, origin.z+offset));
			vertices.push_back(glm::vec3(origin.x+offset, origin.y, origin.z+offset));
			origin.x += offset;
		}
		origin.x = 0.f;
		origin.z += offset;
	}

	glGenVertexArrays(1, &patch.VAO);
	glBindVertexArray(patch.VAO);

	glGenBuffers(1, &patch.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, patch.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glPatchParameteri(GL_PATCH_VERTICES, 4);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	return patch;
}

/*
 * a -- b
 * |	|
 * |	|
 * c -- d
 */
struct mesh gen_quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
	struct mesh quads = {
		.VAO = 0, .VBO = 0, .EBO = 0,
		.mode = GL_TRIANGLE_STRIP,
		.ecount = 0,
		.indexed = false
	};

	const GLfloat positions[] = {
		a.x, a.y, a.z,
		c.x, c.y, c.z,
		b.x, b.y, b.z,
		d.x, d.y, d.z,
	};

	const GLfloat texcoords[] = {
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
	};

	glGenVertexArrays(1, &quads.VAO);
	glBindVertexArray(quads.VAO);

	glGenBuffers(1, &quads.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, quads.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions)+sizeof(texcoords), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), positions);
 	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(texcoords), texcoords);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(positions)));

	return quads;
}

// 3 quads used for grass meshes
struct mesh gen_cardinal_quads(void)
{
	struct mesh quads = {
		.VAO = 0, .VBO = 0, .EBO = 0,
		.mode = GL_TRIANGLES,
		.ecount = 18,
		.indexed = true
	};

	const GLfloat positions[] = {
		1.0, -1.0, 0.0,
		-1.0, -1.0, 0.0,
		-1.0, 1.0, 0.0,
		1.0, 1.0, 0.0,

		0.7, -1.0, 0.7,
		-0.7, -1.0, -0.7,
		-0.7, 1.0, -0.7,
		0.7, 1.0, 0.7,

		0.7, -1.0, -0.7,
		-0.7, -1.0, 0.7,
		-0.7, 1.0, 0.7,
		0.7, 1.0, -0.7,
	};

	const GLfloat texcoords[] = {
		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,

		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,

		1.0, 1.0,
		0.0, 1.0,
		0.0, 0.0,
		1.0, 0.0,
	};

	const GLushort indices[] = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6, 
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,
	};

	glGenVertexArrays(1, &quads.VAO);
	glBindVertexArray(quads.VAO);

	glGenBuffers(1, &quads.EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quads.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

	glGenBuffers(1, &quads.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, quads.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions)+sizeof(texcoords), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), positions);
 	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(texcoords), texcoords);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(positions)));

	return quads;
}

struct mesh gen_mapcube(void)
{
	struct mesh cube = {
		.VAO = 0, .VBO = 0, .EBO = 0,
		.mode = GL_TRIANGLE_STRIP,
		.ecount = 17,
		.indexed = false
	};
	// 8 corners of a cube, side length 2
	const GLfloat positions[] = {
		1.f, 1.f, 1.f,
		1.f, 1.f, -1.f,
		1.f, -1.f, 1.f,
		1.f, -1.f, -1.f,
		-1.f, 1.f, 1.f,
		-1.f, 1.f, -1.f,
		-1.f, -1.f, 1.f,
		-1.f, -1.f, -1.f, 
	};

	// indices for the triangle strips
	const GLushort indices[] = {
		0, 1, 2, 3, 6, 7, 4, 5,
		0xFFFF,
		2, 6, 0, 4, 1, 5, 3, 7
	};

	glGenVertexArrays(1, &cube.VAO);
	glBindVertexArray(cube.VAO);

	glGenBuffers(1, &cube.EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

	glGenBuffers(1, &cube.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, cube.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	return cube;
}

struct mesh create_cube(void)
{
	struct mesh cube = {
		.VAO = 0, .VBO = 0, .EBO = 0,
		.mode = GL_TRIANGLE_STRIP,
		.ecount = 17,
		.indexed = true
	};
	// 8 corners of a cube, side length 2
	const GLfloat positions[] = {
		-1.f, -1.f, -1.f,
		-1.f, -1.f,  1.f,
		-1.f,  1.f, -1.f,
		-1.f,  1.f,  1.f,
		1.f, -1.f, -1.f,
		1.f, -1.f,  1.f,
		1.f,  1.f, -1.f,
		1.f,  1.f,  1.f,
	};

	// indices for the triangle strips
	const GLushort indices[] = {
		0, 1, 2, 3, 6, 7, 4, 5,
		0xFFFF,
		2, 6, 0, 4, 1, 5, 3, 7,
	};

	glGenVertexArrays(1, &cube.VAO);
	glBindVertexArray(cube.VAO);

	glGenBuffers(1, &cube.EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

	glGenBuffers(1, &cube.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, cube.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	return cube;
}

void delete_mesh(const struct mesh *m)
{
	glDeleteBuffers(1, &m->EBO);
	glDeleteBuffers(1, &m->VBO);
	glDeleteVertexArrays(1, &m->VAO);
}

GLuint bind_texture(const struct rawimage *image, GLenum internalformat, GLenum format, GLenum type)
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, image->width, image->height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, format, type, image->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

// generate mip mapped texture
GLuint bind_mipmap_texture(struct byteimage *image, GLenum internalformat, GLenum format, GLenum type)
{
	GLuint texture;

	GLsizei NUM_MIPMAPS = 6;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexStorage2D(GL_TEXTURE_2D, NUM_MIPMAPS, internalformat, image->width, image->height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, format, type, image->data);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void activate_texture(GLenum unit, GLenum target, GLuint texture)
{
	glActiveTexture(unit);
	glBindTexture(target, texture);
}

GLuint load_TGA_cubemap(const char *fpath[6])
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	for (int face = 0; face < 6; face++) {
		GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
		int width, height, nchannels;
		unsigned char *image = stbi_load(fpath[face], &width, &height, &nchannels, 0);
		if (image) {
			glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			stbi_image_free(image);
		} else {
			std::cerr << "cubemap error: failed to load " << fpath[face] << std::endl;
			return 0;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return texture;
}

void instance_static_VAO(GLuint VAO, std::vector<glm::mat4> *transforms)
{
	glBindVertexArray(VAO);
	GLuint VBO;

	auto amount = transforms->size();

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), transforms->data(), GL_STATIC_DRAW);

	const GLuint ATTRIB_START_LOC = 5;

	for (int i = 0; i < amount; i++) {
		size_t size = 4 * sizeof(float);

		glEnableVertexAttribArray(ATTRIB_START_LOC);
		glVertexAttribPointer(ATTRIB_START_LOC, 4, GL_FLOAT, GL_FALSE, 4 * size, BUFFER_OFFSET(0));

		glEnableVertexAttribArray(ATTRIB_START_LOC+1);
		glVertexAttribPointer(ATTRIB_START_LOC+1, 4, GL_FLOAT, GL_FALSE, 4 * size, BUFFER_OFFSET(size));
		glEnableVertexAttribArray(ATTRIB_START_LOC+2);
		glVertexAttribPointer(ATTRIB_START_LOC+2, 4, GL_FLOAT, GL_FALSE, 4 * size, BUFFER_OFFSET(2 * size)); 
		glEnableVertexAttribArray(ATTRIB_START_LOC+3); 
		glVertexAttribPointer(ATTRIB_START_LOC+3, 4, GL_FLOAT, GL_FALSE, 4 * size, BUFFER_OFFSET(3 * size));

		glVertexAttribDivisor(ATTRIB_START_LOC, 1);
		glVertexAttribDivisor(ATTRIB_START_LOC+1, 1);
		glVertexAttribDivisor(ATTRIB_START_LOC+2, 1);
		glVertexAttribDivisor(ATTRIB_START_LOC+3, 1);

		glBindVertexArray(0);
	}

	glDeleteBuffers(1, &VBO);
}

GLuint instance_dynamic_VAO(GLuint VAO, size_t instancecount)
{
	glBindVertexArray(VAO);

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	glBufferData(GL_ARRAY_BUFFER, instancecount * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);

	// loop over each column of the matrix
	const GLuint MATRIX_LOC = 5;
	//size_t row_size = 4 * sizeof(float);
	for (int i = 0; i < 4; i++) {
		glVertexAttribPointer(MATRIX_LOC + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4) * i));
		glEnableVertexAttribArray(MATRIX_LOC + i);
		// make it instanced
		glVertexAttribDivisor(MATRIX_LOC + i, 1);
	}

	return buffer;
}

struct TBO create_TBO(GLsizeiptr size, GLenum internalformat)
{
	struct TBO tbo;

	glGenTextures(1, &tbo.texture);
	glBindTexture(GL_TEXTURE_BUFFER, tbo.texture);

	glGenBuffers(1, &tbo.buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, tbo.buffer);
	glBufferData(GL_TEXTURE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	glTexBuffer(GL_TEXTURE_BUFFER, internalformat, tbo.buffer);

	return tbo;
}


