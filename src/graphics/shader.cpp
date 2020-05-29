#include <cstdlib>
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

static const GLchar *importshader(const char *fpath)
{
	FILE *fp = fopen(fpath, "rb");

	if (!fp) {
		std::cerr << "error: to open file '" << fpath << "'" << std::endl;
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	const auto len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	GLchar *source = new GLchar[len+1];

	fread(source, 1, len, fp);
	fclose(fp);

	source[len] = 0;

	return const_cast<const GLchar*>(source);
}

Shader::Shader(struct shaderinfo *shaders) 
{
	program = loadshaders(shaders);

	if (glIsProgram(program) == GL_FALSE) {
		program = substitute();
	}
}

GLuint Shader::loadshaders(shaderinfo *shaders)
{
	if (shaders == NULL) { return 0; }

	GLuint program = glCreateProgram();

	shaderinfo *entry = shaders;
	while (entry->type != GL_NONE) {
		GLuint shader = glCreateShader(entry->type);

		entry->shader = shader;

		const GLchar *source = importshader(entry->fpath);
		if (source == NULL) {
			for (entry = shaders; entry->type != GL_NONE; ++entry) {
				glDeleteShader(entry->shader);
				entry->shader = 0;
			}
			return 0;
		}

		glShaderSource(shader, 1, &source, NULL);
		delete [] source;

		glCompileShader(shader);

		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLsizei len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

			GLchar *log = new GLchar[len+1];
			glGetShaderInfoLog(shader, len, &len, log);
			std::cerr << "error: shader compilation failed: " << log << std::endl;
			delete [] log;

			return 0;
		}

		glAttachShader(program, shader);

		entry++;
	}

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

		GLchar *log = new GLchar[len+1];
		glGetProgramInfoLog(program, len, &len, log);
		std::cerr << "error: shader linking failed: " << log << std::endl;
		delete [] log;

		for (entry = shaders; entry->type != GL_NONE; ++entry) {
			glDeleteShader(entry->shader);
			entry->shader = 0;
		}

		return 0;
	}

	return program;
}

/* simple hard coded shader in case of trouble */
GLuint Shader::substitute(void)
{
	const GLchar *SUBSTITUTE_VERTEX_SHADER = {
		"#version 440\n"
		"\n"
		"uniform mat4 project, view, model;\n"
		"\n"
		"layout (location = 0) in vec3 position;\n"
		"layout (location = 1) in vec3 normal;\n"
		"layout (location = 2) in vec2 uv;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		" gl_Position = project * view * model * vec4(position, 1.0);\n"
		"}\n"
	};

	const GLchar *SUBSTITUTE_FRAGMENT_SHADER = {
		"#version 440\n"
		"\n"
		"out vec4 fcolor;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		" fcolor = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"}\n"
	};
	GLuint program = glCreateProgram();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &SUBSTITUTE_VERTEX_SHADER, NULL);
	glCompileShader(vertex);
	glAttachShader(program, vertex);

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &SUBSTITUTE_FRAGMENT_SHADER, NULL);
	glCompileShader(fragment);
	glAttachShader(program, fragment);

	glLinkProgram(program);

	return program;
}
