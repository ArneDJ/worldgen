#include <iostream>
#include <vector>
#include <random>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "effects.h"

void Particles::init_buffers(void)
{
	std::random_device rd;
	std::mt19937 gen(rd());

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// init position buffer
	glGenBuffers(2, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);

	glm::vec4 *positions = (glm::vec4 *)glMapBufferRange(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	std::uniform_real_distribution<> pos(-10.f, 10.f);
	std::uniform_real_distribution<> life(0.f, 1.f);

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		positions[i] = glm::vec4(glm::vec3(pos(gen), pos(gen), pos(gen)), life(gen));
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// init velocity buffer
	glBindBuffer(GL_ARRAY_BUFFER, velocity_buffer);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(glm::vec4), NULL, GL_DYNAMIC_COPY);

	glm::vec4 *velocities = (glm::vec4 *)glMapBufferRange(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	std::uniform_real_distribution<> vel(-0.1f, 0.1f);

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		velocities[i] = glm::vec4(glm::vec3(vel(gen), vel(gen), vel(gen)), 0.0f);
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glGenTextures(2, tbos);
	glBindTexture(GL_TEXTURE_BUFFER, position_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, position_buffer);

	//glGenTextures(1, &velocity_tbo);
	glBindTexture(GL_TEXTURE_BUFFER, velocity_tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, velocity_buffer);

	// attractor buffer
	glGenBuffers(1, &attractor_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, attractor_buffer);
	glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(glm::vec4), NULL, GL_STATIC_DRAW);

	for (int i = 0; i < MAX_ATTRACTORS; i++) {
		attractor_masses[i] = 0.5f + life(gen) * 0.5f;
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, attractor_buffer);
}

void Particles::update_particles(const Shader *compute, float time, float delta)
{
	// update the buffer containing the attractor positions and masses
	glBindBuffer(GL_UNIFORM_BUFFER, attractor_buffer);

	glm::vec4 *attractors = (glm::vec4 *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 32 * sizeof(glm::vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	for (int i = 0; i < 32; i++) {
  attractors[i] = glm::vec4(sinf(time * (float)(i + 4) * 7.5f * 20.0f) * 50.0f,
   cosf(time * (float)(i + 7) * 3.9f * 20.0f) * 50.0f,
   sinf(time * (float)(i + 3) * 5.3f * 20.0f) * cosf(time * (float)(i + 5) * 9.1f) * 100.0f,
   attractor_masses[i]);
 	}

	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// If dt is too large, the system could explode, so cap it
	if (delta >= 2.0f) {
		delta = 2.0f;
	}

	// Activate the compute program and bind the position and velocity buffers
	compute->bind();

	glBindImageTexture(0, velocity_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, position_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	// Set delta time
	compute->uniform_float("dt", delta);
	// Dispatch
	glDispatchCompute(PARTICLE_GROUP_COUNT, 1, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
