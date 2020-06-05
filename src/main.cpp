#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "external/imgui/imgui.h"
#include "external/imgui/imgui_impl_sdl.h"
#include "external/imgui/imgui_impl_opengl3.h"

#include "graphics/imp.h"
#include "graphics/dds.h"
#include "graphics/glwrapper.h"
#include "graphics/shader.h"
#include "graphics/camera.h"
#include "graphics/voronoi.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FOV 90.f
#define NEAR_CLIP 0.1f
#define FAR_CLIP 1600.f

#define FOG_DENSITY 0.015f

#define SEA_LEVEL 0.43f
#define MOUNTAIN_LEVEL 0.65f
#define NSITES 256*256

class Skybox {
public:
	Skybox(GLuint cubemapbind)
	{
		cubemap = cubemapbind;
		cube = gen_mapcube();
	};
	void display(void) const;
private:
	GLuint cubemap;
	struct mesh cube;
};

void Skybox::display(void) const
{
	glDepthFunc(GL_LEQUAL);
	activate_texture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, cubemap);
	glBindVertexArray(cube.VAO);
	glDrawElements(cube.mode, cube.ecount, GL_UNSIGNED_SHORT, NULL);
	glDepthFunc(GL_LESS);
};

GLuint bind_byte_texture(const struct byteimage *image, GLenum internalformat, GLenum format, GLenum type)
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, image->width, image->height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, format, type, image->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

Shader base_shader(const char *vertpath, const char *fragpath)
{
	struct shaderinfo pipeline[] = {
		{GL_VERTEX_SHADER, vertpath},
		{GL_FRAGMENT_SHADER, fragpath},
		{GL_NONE, NULL}
	};

	Shader shader(pipeline);

	shader.bind();

	glm::mat4 model = glm::mat4(1.f);
	shader.uniform_mat4("model", model);
	shader.uniform_vec3("fogcolor", glm::vec3(0.46, 0.7, 0.99));
	shader.uniform_float("fogfactor", FOG_DENSITY);

	return shader;
}

Shader skybox_shader(void)
{
	struct shaderinfo pipeline[] = {
		{GL_VERTEX_SHADER, "shaders/skybox.vert"},
		{GL_FRAGMENT_SHADER, "shaders/skybox.frag"},
		{GL_NONE, NULL}
	};

	Shader shader(pipeline);

	shader.bind();

	const float aspect = (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT;
	glm::mat4 project = glm::perspective(glm::radians(FOV), aspect, NEAR_CLIP, FAR_CLIP);
	shader.uniform_mat4("project", project);

	return shader;
}

Shader compute_shader(void)
{
	struct shaderinfo pipeline[] = {
	{GL_COMPUTE_SHADER, "shaders/particle.comp"},
	{GL_NONE, NULL}
	};

	Shader shader(pipeline);

	return shader;
}

Skybox init_skybox(void)
{
	const char *CUBEMAP_TEXTURES[6] = {
	"media/textures/skybox/dust_ft.tga",
	"media/textures/skybox/dust_bk.tga",
	"media/textures/skybox/dust_up.tga",
	"media/textures/skybox/dust_dn.tga",
	"media/textures/skybox/dust_rt.tga",
	"media/textures/skybox/dust_lf.tga",
	};

	GLuint cubemap = load_TGA_cubemap(CUBEMAP_TEXTURES);
	Skybox skybox { cubemap };

	return skybox;
}

static inline void start_imguiframe(SDL_Window *window)
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

static void init_imgui(SDL_Window *window, SDL_GLContext glcontext)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, glcontext);
	ImGui_ImplOpenGL3_Init("#version 430");
}

struct byteimage height_texture(long seed)
{
	const size_t size = 1024;

	struct byteimage image = {
		.data = new unsigned char[size*size],
		.nchannels = 1,
		.width = size,
		.height = size,
	};
	heightmap_image(&image, seed, 0.002f, 200.f);

	return image;
}

enum TILE_RELIEF_TYPE {
	WATER,
	PLAIN,
	HILLS,
	MOUNTAIN,
};

enum TILE_TEMPERATURE {
	COLD,
	TEMPERATE,
	WARM,
};

enum TILE_VEGETATION {
	ARID,
	DRY,
	HUMID,
};

enum TILE_BIOME {
	OCEAN,
	SEA,
	MARSH,
	FOREST,
	TAIGA,
	SAVANNA,
	STEPPE,
	DESERT,
};

struct tile {
	enum TILE_RELIEF_TYPE relief;
	enum TILE_TEMPERATURE temperature;
	enum TILE_VEGETATION vegetation;
	enum TILE_BIOME biome;
	bool coast;
	bool river;
	const struct cell *site;
};

enum TILE_RELIEF_TYPE sample_relief(float height)
{
	enum TILE_RELIEF_TYPE relief = WATER;

	if (height < SEA_LEVEL) {
		relief = WATER;
	}
	if (height > MOUNTAIN_LEVEL) {
		relief = MOUNTAIN;
	}
	if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
		std::random_device rd;
		std::mt19937 gen(rd());
		float p = glm::smoothstep(SEA_LEVEL, MOUNTAIN_LEVEL, height);
		std::bernoulli_distribution d(p);
		if (d(gen) == false) {
			relief = PLAIN;
		} else {
			relief = HILLS;
		}
	}

	return relief;
}

void gen_cells(struct byteimage *image, const struct byteimage *heightimage, const struct byteimage *continentimage, const struct byteimage *rainimage)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	// generate set of points based on elevation and rainfall
	std::vector<glm::vec2> points;

	for (int i = 0; i < NSITES; i++) {
		glm::vec2 point = glm::vec2(dis(gen), dis(gen));
		int x = int(point.x);
		int y = int(point.y);
		float height = sample_byte_height(x, y, heightimage);
		float rain = sample_byte_height(x, y, rainimage);
		float p = 1.f;
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			p = 1.f;
		} else if (height < SEA_LEVEL) {
			p = 0.05f;
		} else {
			p = 0.25f;
		}
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			p = glm::mix(p, rain, 0.75f);
		}
		std::bernoulli_distribution d(p);
		if (d(gen) == true) {
			points.push_back(point);
		}
	}

	Voronoi voronoi = { points, image->width, image->height };

	std::vector<struct tile> tiles;

	for (auto &cell : voronoi.cells) {
		int x = int(cell.center.x);
		int y = int(cell.center.y);
		float height = sample_byte_height(x, y, heightimage);
		enum TILE_RELIEF_TYPE relief = sample_relief(height);
		struct tile t = {
			.relief = relief,
			.temperature = COLD,
			.vegetation = ARID,
			.biome = OCEAN,
			.coast = false,
			.river = false,
			.site = &cell,
		};

		tiles.push_back(t);
	}

	glm::vec3 watercolor = {0.2f, 0.2f, 0.95f};
	glm::vec3 plaincolor = {0.5f, 0.7f, 0.4f};
	glm::vec3 hillscolor = {0.7f, 0.7f, 0.5f};
	glm::vec3 mountainscolor = {0.5f, 0.5f, 0.5f};

	for (auto &t : tiles) {
		glm::vec3 color = {1.f, 1.f, 1.f};
		switch (t.relief) {
		case WATER: color = watercolor; break;
		case PLAIN: color = plaincolor; break;
		case HILLS: color = hillscolor; break;
		case MOUNTAIN: color = mountainscolor; break;
		};

		unsigned char c[3];
		c[0] = 255 * color.x;
		c[1] = 255 * color.y;
		c[2] = 255 * color.z;
		for (auto &border : t.site->borders) {
			draw_triangle(t.site->center.x, t.site->center.y, border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, c);
			//draw_line(border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, white);
		}
	}

	unsigned char sitecolor[3] = {255, 255, 255};
	unsigned char linecolor[3] = {255, 255, 255};
	unsigned char delacolor[3] = {255, 0, 0};
	unsigned char blue[3] = {0, 0, 255};
	unsigned char white[3] = {255, 255, 255};
	/*

	for (auto &cell : voronoi.cells) {
		int x = int(cell.center.x);
		int y = int(cell.center.y);
		float height = sample_byte_height(x, y, heightimage);
		unsigned char rcolor[3];
		rcolor[0] = 255;
		rcolor[1] = 0;
		rcolor[2] = 0;

		if (height < SEA_LEVEL) {
		 	height = 0.25f;
			rcolor[0] = 64; rcolor[1] = 64; rcolor[2] = 255;
		} else if (height > MOUNTAIN_LEVEL) {
			height = 1.f;
			rcolor[0] = 200; rcolor[1] = 200; rcolor[2] = 200;
		} else {
			height = 0.75f;
			rcolor[0] = 64; rcolor[1] = 200; rcolor[2] = 64;
		}
		for (auto &border : cell.borders) {
			draw_triangle(cell.center.x, cell.center.y, border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, rcolor);
			//draw_line(border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, white);
		}
	}
	*/

	std::uniform_int_distribution<int> distro(0, voronoi.corners.size() - 1);
	for (int i = 0; i < 500; i++) {
		std::vector<struct corner*> river;
		bool rejected = true;
		int start = distro(gen);
		struct corner *c = &voronoi.corners[start];
		float height = sample_byte_height(c->position.x, c->position.y, heightimage);
		if (height > 0.6f) {
			// find the lowest neighbor
			int tick = 0;
			struct corner *previous = c;
			while (height > SEA_LEVEL && tick < NSITES) {
			river.push_back(c);
			height = sample_byte_height(c->position.x, c->position.y, heightimage);
			float min = 1.f;
			struct corner *neighbor = nullptr;
			for (auto &n : c->adjacent) {
				if (n != previous) {
				float neighborheight = sample_byte_height(n->position.x, n->position.y, heightimage);
				if (neighborheight < min) { 
					neighbor = n;
					min = neighborheight;
				}
				}
			}
			if (neighbor != nullptr) {
				previous = c;
				c = neighbor;
			} else {
				break;
			}
			tick++;
			if (height < SEA_LEVEL) {
				rejected = false;
			}
			}

			if (rejected == false) {
				for (int i = 0; i < river.size(); i++) {
					if (i+1 < river.size()) {
						draw_line(river[i]->position.x, river[i]->position.y, river[i+1]->position.x, river[i+1]->position.y, image->data, image->width, image->height, image->nchannels, blue);
					}
				}
			}
		}
	}

}

GLuint voronoi_texture(const struct byteimage *heightimage, const struct byteimage *continentimage, const struct byteimage *rainimage)
{
	const size_t size = 1024;

	struct byteimage image = {
		.data = new unsigned char[size*size*3],
		.nchannels = 3,
		.width = size,
		.height = size,
	};

	gen_cells(&image, heightimage, continentimage, rainimage);

	GLuint texture = bind_byte_texture(&image, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

	return texture;
}

struct byteimage temperature_texture(long seed)
{
	const size_t size = 1024;

	struct byteimage image = {
		.data = new unsigned char[size*size],
		.nchannels = 1,
		.width = size,
		.height = size,
	};

	gradient_image(&image, seed, 100.f);

	return image;
}

struct byteimage continent_texture(const struct byteimage *heightimage)
{
	struct byteimage image = {
		.data = new unsigned char[heightimage->width*heightimage->height],
		.nchannels = heightimage->nchannels,
		.width = heightimage->width,
		.height = heightimage->height,
	};

	size_t index = 0;
	for (int i = 0; i < heightimage->height; i++) {
		for (int j = 0; j < heightimage->width; j++) {
			float height = heightimage->data[index] / 255.f;
			height = height < SEA_LEVEL ? 0.f : 1.f;
			image.data[index++] = 255.f * height;
		}
	}

	return image;
}

struct byteimage rainfall_texture(const struct byteimage *tempimage, long seed)
{
	const size_t size = tempimage->width * tempimage->height * tempimage->nchannels;

	struct byteimage image = {
		.data = new unsigned char[size],
		.nchannels = tempimage->nchannels,
		.width = tempimage->width,
		.height = tempimage->height,
	};

	heightmap_image(&image, seed, 0.002f, 200.f);

	size_t index = 0;
	for (auto i = 0; i < size; i++) {
			float height = image.data[index] / 255.f;
			height = height < 0.5 ? 0.f : 1.f;
			image.data[index++] = 255.f * (1.f - height);
	}

	gauss_blur_image(&image, 50.f);

	index = 0;
	for (auto i = 0; i < size; i++) {
			float temperature = 1.f - (tempimage->data[index] / 255.f);
			float rainfall = image.data[index] / 255.f;
			rainfall = glm::mix(rainfall, temperature*temperature, 1.f - temperature);
			image.data[index++] = 255.f * rainfall;
	}

	return image;
}

void run_worldgen(SDL_Window *window)
{
	SDL_SetRelativeMouseMode(SDL_TRUE);

	Skybox skybox = init_skybox();
	Shader skybox_program = skybox_shader();
	Shader map_program = base_shader("shaders/map.vert", "shaders/map.frag");

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<long> dis;
	long seed = dis(gen);

	struct byteimage heightimage = height_texture(seed);
	GLuint heightmap = bind_byte_texture(&heightimage, GL_R8, GL_RED, GL_UNSIGNED_BYTE);


	struct byteimage continentimage = continent_texture(&heightimage);
	GLuint continent = bind_byte_texture(&continentimage, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	struct byteimage temperatureimage = temperature_texture(seed);
	GLuint temperature = bind_byte_texture(&temperatureimage, GL_R8, GL_RED, GL_UNSIGNED_BYTE);

	struct byteimage rainimage = rainfall_texture(&temperatureimage, seed);
	GLuint rainfall = bind_byte_texture(&rainimage, GL_R8, GL_RED, GL_UNSIGNED_BYTE);

	GLuint voronoi = voronoi_texture(&heightimage, &continentimage, &rainimage);

	Camera cam = { 
		glm::vec3(300.f, 8.f, 8.f),
		FOV,
		float(WINDOW_WIDTH) / float(WINDOW_HEIGHT),
		NEAR_CLIP,
		FAR_CLIP
	};

	struct mesh map = gen_quad(glm::vec3(0.f, 100.f, 0.f), glm::vec3(100.f, 100.f, 0.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(100.f, 0.f, 0.f));

	float start = 0.f;
 	float end = 0.f;
	unsigned long frames = 0;
	unsigned int msperframe = 0;

	bool running = true;
	SDL_Event event;
	while (running == true) {
		start = 0.001f * float(SDL_GetTicks());
		const float delta = start - end;

		while(SDL_PollEvent(&event));
  		const Uint8 *keystates = SDL_GetKeyboardState(NULL);
		if (event.type == SDL_QUIT) { running = false; }
		if (keystates[SDL_SCANCODE_ESCAPE]) { running = false; }

		cam.update_center(delta);
		cam.update_view();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

		glm::mat4 VP = cam.project * cam.view;
		skybox_program.uniform_mat4("view", cam.view);
		map_program.uniform_mat4("VIEW_PROJECT", VP);

		glDisable(GL_CULL_FACE);
		map_program.bind();
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, heightmap);
		glBindVertexArray(map.VAO);
		glDrawArrays(map.mode, 0, map.ecount);

		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(100.f, 0.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, continent);
		glDrawArrays(map.mode, 0, map.ecount);
		glEnable(GL_CULL_FACE);

		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(200.f, 0.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, rainfall);
		glDrawArrays(map.mode, 0, map.ecount);
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(300.f, 0.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, temperature);
		glDrawArrays(map.mode, 0, map.ecount);
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(400.f, 0.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, voronoi);
		glDrawArrays(map.mode, 0, map.ecount);
		glEnable(GL_CULL_FACE);

		skybox_program.bind();
		skybox.display();

		// debug UI
		start_imguiframe(window);

		ImGui::Begin("Debug");
		ImGui::SetWindowSize(ImVec2(400, 200));
		ImGui::Text("%d ms per frame", msperframe);
		ImGui::Text("camera position: %.2f, %.2f, %.2f", cam.eye.x, cam.eye.y, cam.eye.z);

		//if (ImGui::Button("Exit")) { running = false; }

		ImGui::End();

		// Render dear imgui into screen
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
		end = start;
		frames++;
		if (frames % 100 == 0) { 
			msperframe = (unsigned int)(delta*1000); 
		}
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow("terraingen", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("Could not create window: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(EXIT_FAILURE);
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xFFFF);
	glEnable(GL_DEPTH_TEST);
 	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	init_imgui(window, glcontext);

	run_worldgen(window);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	exit(EXIT_SUCCESS);
}
