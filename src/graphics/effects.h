enum {
	PARTICLE_GROUP_SIZE = 128,
	PARTICLE_GROUP_COUNT = 8000,
	PARTICLE_COUNT = (PARTICLE_GROUP_SIZE * PARTICLE_GROUP_COUNT),
	MAX_ATTRACTORS = 64
};

class Particles {
public:
	Particles(void)
	{
		init_buffers();
	}
	void update_particles(const Shader *compute, float time, float delta);
	void display(const Shader *shader)
	{
		shader->bind();
		glBindVertexArray(VAO);
		glBlendFunc(GL_ONE, GL_ONE);
		glPointSize(2.f);
		glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
		// reset blend func
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
private:
	GLuint VAO;
	GLuint attractor_buffer;
	// Posisition and velocity buffers
	union {
		struct {
			GLuint position_buffer;
			GLuint velocity_buffer;
		};
		GLuint buffers[2];
	};

	// TBOs
	union {
		struct {
			GLuint position_tbo;
			GLuint velocity_tbo;
		};
		GLuint tbos[2];
	};

	// Mass of the attractors
	float attractor_masses[MAX_ATTRACTORS];
private:
	void init_buffers(void);
};

