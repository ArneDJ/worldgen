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
#include "external/fastnoise/FastNoise.h"

#include "graphics/imp.h"
#include "graphics/dds.h"
#include "graphics/glwrapper.h"
#include "graphics/shader.h"
#include "graphics/camera.h"
#include "worldmap/voronoi.h"
#include "worldmap/terraform.h"
#include "worldmap/tilemap.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FOV 90.f
#define NEAR_CLIP 0.1f
#define FAR_CLIP 1600.f

#define FOG_DENSITY 0.015f
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

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

Shader worldmap_shader(void)
{
	struct shaderinfo pipeline[] = {
		{GL_VERTEX_SHADER, "shaders/worldmap.vert"},
		{GL_TESS_CONTROL_SHADER, "shaders/worldmap.tesc"},
		{GL_TESS_EVALUATION_SHADER, "shaders/worldmap.tese"},
		{GL_FRAGMENT_SHADER, "shaders/worldmap.frag"},
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

void cellnoise_image(float *image, size_t sidelength, long seed, float freq)
{
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Add);
	cellnoise.SetGradientPerturbAmp(10.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float height = cellnoise.GetNoise(x, y) / max;
			image[index++] = height;
		}
	}
}

void badlands_image(float *image, size_t sidelength, long seed, float freq)
{
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Sub);
	cellnoise.SetGradientPerturbAmp(10.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float height = cellnoise.GetNoise(x, y) / max;
			height = sqrtf(height);
			if (height > 0.1f && height < 0.2f) { 
				height = glm::mix(0.1f, 0.2f, glm::smoothstep(0.15f, 0.16f, height));
			}
			if (height > 0.25f && height < 0.5f) { 
				height = glm::mix(0.25f, 0.5f, glm::smoothstep(0.35f, 0.4f, height));
			}
			image[index++] = height*height;
		}
	}
}

GLuint voronoi_texture(const Tilemap *map)
{
	const size_t size = 4096;

	struct byteimage image = {
		.data = new unsigned char[size*size*3],
		.nchannels = 3,
		.width = size,
		.height = size,
	};

	glm::vec3 ocean = {0.2f, 0.2f, 0.95f};
	glm::vec3 forest = {0.2f, 1.f, 0.2f};
	glm::vec3 taiga = {0.2f, 0.95f, 0.6f};
	glm::vec3 grassland = {0.6f, 0.9f, 0.2f};
	glm::vec3 savanna = {1.f, 0.5f, 0.f};
	glm::vec3 steppe = {0.7f, 0.8f, 0.2f};
	glm::vec3 desert = {0.8f, 0.9f, 0.2f};
	glm::vec3 alpine = {0.8f, 0.8f, 0.8f};
	glm::vec3 badlands = {1.f, 0.8f, 0.8f};
	glm::vec3 floodplain = {0.1f, 0.5f, 0.f};
	glm::vec3 marsh = {0.1f, 0.5f, 0.4f};

	for (auto &t : map->tiles) {
		glm::vec3 color = {1.f, 1.f, 1.f};
		switch (t.biome) {
		case OCEAN: color = ocean; break;
		case FOREST: color = forest; break;
		case TAIGA: color = taiga; break;
		case SAVANNA: color = savanna; break;
		case STEPPE: color = steppe; break;
		case DESERT: color = desert; break;
		case ALPINE: color = alpine; break;
		case BADLANDS: color = badlands; break;
		case GRASSLAND: color = grassland; break;
		case FLOODPLAIN: color = floodplain; break;
		case MARSH: color = marsh; break;
		};

		unsigned char c[3];
		c[0] = 255 * color.x;
		c[1] = 255 * color.y;
		c[2] = 255 * color.z;
		for (auto &border : t.site->borders) {
			draw_triangle(t.site->center.x, t.site->center.y, border.x, border.y, border.z, border.w, image.data, image.width, image.height, image.nchannels, c);
		}
	}

	unsigned char sitecolor[3] = {255, 255, 255};
	unsigned char linecolor[3] = {255, 255, 255};
	unsigned char delacolor[3] = {255, 0, 0};
	unsigned char blue[3] = {0, 0, 255};
	unsigned char white[3] = {255, 255, 255};

	for (const auto &river : map->rivers) {
		for (const auto &segment : river.segments) {
			draw_line(segment.a->v->position.x, segment.a->v->position.y, segment.b->v->position.x, segment.b->v->position.y, image.data, image.width, image.height, image.nchannels, blue);
		}
	}

	GLuint texture = bind_byte_texture(&image, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);

	return texture;
}

struct byteimage relief_mask(const Tilemap *map, enum RELIEF relief, size_t width, size_t height, float blur)
{
	struct byteimage image = {
		.data = new unsigned char[width*height],
		.nchannels = 1,
		.width = width,
		.height = height,
	};

	for (int i = 0; i < width*height; i++) {
		image.data[i] = 0;
	}

	const float ratio = float(width) / float(4096);

	unsigned char mask[3] = {255, 255, 255};

	for (const auto &tile : map->tiles) {
		if (tile.relief == relief) {
			for (const auto &border : tile.site->borders) {
				draw_triangle(ratio*tile.site->center.x, ratio*tile.site->center.y, ratio*border.x, ratio*border.y, ratio*border.z, ratio*border.w, image.data, image.width, image.height, image.nchannels, mask);
			}
		}
	}

	gauss_blur_image(&image, blur);

	return image;
}

struct byteimage biome_mask(const Tilemap *map, enum BIOME biome, size_t width, size_t height, float blur)
{
	struct byteimage image = {
		.data = new unsigned char[width*height],
		.nchannels = 1,
		.width = width,
		.height = height,
	};

	for (int i = 0; i < width*height; i++) {
		image.data[i] = 0;
	}

	const float ratio = float(width) / float(4096);

	unsigned char mask[3] = {255, 255, 255};

	for (const auto &tile : map->tiles) {
		if (tile.biome == biome) {
			for (const auto &border : tile.site->borders) {
				draw_triangle(ratio*tile.site->center.x, ratio*tile.site->center.y, ratio*border.x, ratio*border.y, ratio*border.z, ratio*border.w, image.data, image.width, image.height, image.nchannels, mask);
			}
		}
	}

	gauss_blur_image(&image, blur);

	return image;
}

// all images should only have a single color channel
struct floatimage gen_topology(const struct floatimage *heightmap, const Tilemap *map, long seed)
{
	const size_t width = 2048;
	const size_t height = 2048;
	struct byteimage water = relief_mask(map, WATER, width, height, 1.f);
	struct byteimage mountain = relief_mask(map, MOUNTAIN, width, height, 5.f);

	struct floatimage image = {
		.data = new float[width*height],
		.nchannels = 1,
		.width = width,
		.height = height,
	};

	const float ratio = float(heightmap->width) / float(width);

	{
		unsigned int index = 0; 
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				float height = sample_floatimage(ratio*j, ratio*i, 0, heightmap);
				image.data[index++] = 0.5f * height;
			}
		}
	}

	struct floatimage mountainmap = {
		.data = new float[width*height],
		.nchannels = 1,
		.width = width,
		.height = height,
	};

	cellnoise_image(mountainmap.data, mountainmap.width, seed, 0.04f);

	struct floatimage badlands = {
		.data = new float[width*height],
		.nchannels = 1,
		.width = width,
		.height = height,
	};

	badlands_image(badlands.data, badlands.width, seed, 0.02f);

	struct byteimage badlandmask = biome_mask(map, BADLANDS, width, height, 1.f);
	for (int i = 0; i < width*height; i++) {
		float mask = badlandmask.data[i] / 255.f;
		float height = glm::mix(mountainmap.data[i], badlands.data[i], mask);
		mountainmap.data[i] = height;
	}

	unsigned int index = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			float mask = sample_byteimage(j, i, 0, &mountain);
			float watermask = sample_byteimage(j, i, 0, &water);
			float ridge = (0.25f*mountainmap.data[index]) + 0.4f;
			float height = glm::mix(image.data[index], ridge, mask*mask*mask*mask);
			height = glm::mix(height, 0.75f*height, watermask);
			image.data[index++] = height;
		}
	}

	delete [] mountainmap.data;
	delete [] badlands.data;
	delete [] water.data;
	delete [] mountain.data;

	return image;
}

void run_worldgen(SDL_Window *window)
{
	SDL_SetRelativeMouseMode(SDL_TRUE);

	Skybox skybox = init_skybox();
	Shader skybox_program = skybox_shader();
	Shader map_program = base_shader("shaders/map.vert", "shaders/map.frag");
	Shader worldmap_program = worldmap_shader();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<long> dis;
	long seed = dis(gen);

	Terraform terraform = { 2048, seed, 0.001f, };

	GLuint heightmap = bind_texture(&terraform.heightmap, GL_R8, GL_RED, GL_FLOAT);
	GLuint temperature = bind_byte_texture(&terraform.temperature, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	GLuint rainfall = bind_byte_texture(&terraform.rainfall, GL_R8, GL_RED, GL_UNSIGNED_BYTE);

	Tilemap tilecells; 
	tilecells.gen_tiles(4096, &terraform.heightmap, &terraform.rainfall, &terraform.temperature);
	GLuint voronoi = voronoi_texture(&tilecells);

	struct floatimage topology = gen_topology(&terraform.heightmap, &tilecells, seed);
	GLuint reliefheight = bind_texture(&topology, GL_R8, GL_RED, GL_FLOAT);
	struct floatimage normalimage = gen_normalmap(&topology);
	GLuint normalmap = bind_texture(&normalimage, GL_RGB8, GL_RGB, GL_FLOAT);

	struct mesh worldmap_mesh = gen_patch_grid(32, 32.f);
	worldmap_program.uniform_float("mapscale", 1.f / (32.f*32.f));
	worldmap_program.uniform_float("amplitude", 48.f);

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
		worldmap_program.uniform_mat4("VIEW_PROJECT", VP);

		worldmap_program.bind();
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, reliefheight);
		activate_texture(GL_TEXTURE1, GL_TEXTURE_2D, voronoi);
		activate_texture(GL_TEXTURE2, GL_TEXTURE_2D, normalmap);
		glBindVertexArray(worldmap_mesh.VAO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		 glPatchParameteri(GL_PATCH_VERTICES, 4);
		 glDrawArrays(GL_PATCHES, 0, worldmap_mesh.ecount);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDisable(GL_CULL_FACE);
		map_program.bind();
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(0.f, 32.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, heightmap);
		glBindVertexArray(map.VAO);
		glDrawArrays(map.mode, 0, map.ecount);

		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(100.f, 32.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, rainfall);
		glDrawArrays(map.mode, 0, map.ecount);
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(200.f, 32.f, 0.f)));
		activate_texture(GL_TEXTURE0, GL_TEXTURE_2D, temperature);
		glDrawArrays(map.mode, 0, map.ecount);
		map_program.uniform_mat4("model", glm::translate(glm::mat4(1.f), glm::vec3(400.f, 32.f, 0.f)));
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
