#include <iostream>
	#include <random>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "../external/fastnoise/FastNoise.h"

#define JC_VORONOI_IMPLEMENTATION
#include "../external/voronoi/jc_voronoi.h"

#include "imp.h"

enum {
	RED_CHANNEL = 1,
	RG_CHANNEL = 2,
	RGB_CHANNEL = 3,
	RGBA_CHANNEL = 4
};

static inline float sample_height(int x, int y, const struct floatimage *image)
{
	if (x < 0 || y < 0 || x > (image->width-1) || y > (image->height-1)) {
		return 0.f; 
	}

	int index = y * image->width * image->nchannels + x * image->nchannels;

	//return image->data[index] / 255.f;
	return image->data[index];
}

static inline float sample_byte_height(int x, int y, const struct byteimage *image)
{
	if (x < 0 || y < 0 || x > (image->width-1) || y > (image->height-1)) {
		return 0.f; 
	}

	int index = y * image->width * image->nchannels + x * image->nchannels;

	return image->data[index] / 255.f;
}

static glm::vec3 filter_normal(int x, int y, const struct floatimage *image)
{
	const float strength = 32.f; // sobel filter strength

	float T = sample_height(x, y + 1, image);
	float TR = sample_height(x + 1, y + 1, image);
	float TL = sample_height(x - 1, y + 1, image);
	float B = sample_height(x, y - 1, image);
	float BR = sample_height(x + 1, y - 1, image);
	float BL = sample_height(x - 1, y - 1, image);
	float R = sample_height(x + 1, y, image);
	float L = sample_height(x - 1, y, image);

	// sobel filter
	const float dX = (TR + 2.f * R + BR) - (TL + 2.f * L + BL);
	const float dZ = (BL + 2.f * B + BR) - (TL + 2.f * T + TR);
	const float dY = 1.f / strength;

	glm::vec3 normal(-dX, dY, dZ);
	normal = glm::normalize(normal);
	// convert to positive values to store in a texture
	normal.x = (normal.x + 1.f) / 2.f;
	normal.y = (normal.y + 1.f) / 2.f;
	normal.z = (normal.z + 1.f) / 2.f;

	return normal;
}

float sample_image(int x, int y, const struct floatimage *image, unsigned int channel)
{
	if (image->data == nullptr) {
		std::cerr << "error: no image data\n";
		return 0.f;
	}
	if (channel > image->nchannels) {
		std::cerr << "error: invalid channel to sample\n";
		return 0.f;
	}
	if (x < 0 || y < 0 || x > (image->width-1) || y > (image->height-1)) {
		return 0.f;
	}

	int index = y * image->width * image->nchannels + x * image->nchannels;

	//return image->data[index+channel] / 255.f;
	return image->data[index+channel];
}

struct floatimage gen_normalmap(const struct floatimage *heightmap)
{
	struct floatimage normalmap = {
		.data = new float[heightmap->width * heightmap->height * RGB_CHANNEL],
		.nchannels = RGB_CHANNEL,
		.width = heightmap->width,
		.height = heightmap->height
	};

	for (int x = 0; x < heightmap->width; x++) {
		for (int y = 0; y < heightmap->height; y++) {
			const unsigned int index = y * normalmap.width * RGB_CHANNEL + x * RGB_CHANNEL;
			const glm::vec3 normal = filter_normal(x, y, heightmap);
			normalmap.data[index] = normal.x;
			normalmap.data[index+1] = normal.y;
			normalmap.data[index+2] = normal.z;
			/*
			normalmap.data[index] = normal.x * 255.f;
			normalmap.data[index+1] = normal.y * 255.f;
			normalmap.data[index+2] = normal.z * 255.f;
			*/
		}
	}

	return normalmap;
}

void billow_3D_image(unsigned char *image, size_t sidelength, float frequency, float cloud_distance)
{
	FastNoise billow;
	billow.SetNoiseType(FastNoise::SimplexFractal);
	billow.SetFractalType(FastNoise::Billow);
	billow.SetFrequency(frequency);
	billow.SetFractalOctaves(6);
	billow.SetFractalLacunarity(2.0f);

	const float space = cloud_distance; // space between the clouds

	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			for (int k = 0; k < sidelength; k++) {
				float p = (billow.GetNoise(i, j, k) + 1.f) / 2.f;
				p = p - space;
				image[index++] = glm::clamp(p, 0.f, 1.f) * 255.f;
			}
		}
	}

}

void terrain_image(float *image, size_t sidelength, long seed, float freq, float mountain_amp, float field_amp)
{
	// detail
	FastNoise billow;
	billow.SetSeed(seed);
	billow.SetNoiseType(FastNoise::SimplexFractal);
	billow.SetFractalType(FastNoise::Billow);
	billow.SetFrequency(0.01f);
	billow.SetFractalOctaves(6);
	billow.SetFractalLacunarity(2.0f);
	billow.SetGradientPerturbAmp(40.f);

	// ridges
	FastNoise cellnoise;
	cellnoise.SetSeed(seed);
	cellnoise.SetNoiseType(FastNoise::Cellular);
	cellnoise.SetCellularDistanceFunction(FastNoise::Euclidean);
	cellnoise.SetFrequency(0.01f*freq);
	cellnoise.SetCellularReturnType(FastNoise::Distance2Add);
	cellnoise.SetGradientPerturbAmp(30.f);

	// mask perturb
	FastNoise perturb;
	perturb.SetSeed(seed);
	perturb.SetNoiseType(FastNoise::SimplexFractal);
	perturb.SetFractalType(FastNoise::FBM);
	perturb.SetFrequency(0.002f*freq);
	perturb.SetFractalOctaves(5);
	perturb.SetFractalLacunarity(2.0f);
	perturb.SetGradientPerturbAmp(300.f);

	float max = 1.f;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float val = cellnoise.GetNoise(x, y);
			if (val > max) { max = val; }
		}
	}

	const glm::vec2 center = glm::vec2(0.5f*float(sidelength), 0.5f*float(sidelength));
	unsigned int index = 0;
	for (int i = 0; i < sidelength; i++) {
		for (int j = 0; j < sidelength; j++) {
			float x = i; float y = j;
			billow.GradientPerturbFractal(x, y);
			float detail = 1.f - (billow.GetNoise(x, y) + 1.f) / 2.f;

			x = i; y = j;
			cellnoise.GradientPerturbFractal(x, y);
			float ridge = cellnoise.GetNoise(x, y) / max;

			x = i; y = j;
			perturb.GradientPerturbFractal(x, y);

			// add detail
			float height = glm::mix(detail, ridge, 0.9f);

			// aply mask
			float mask = glm::distance(center, glm::vec2(float(x), float(y))) / float(0.5f*sidelength);
			mask = glm::smoothstep(0.4f, 0.8f, mask);
			mask = glm::clamp(mask, field_amp, mountain_amp);

			height *= mask;

			if (i > (sidelength-4) || j > (sidelength-4)) {
				image[index++] = 0.f;
			} else {
				//image[index++] = height * 255.f;
				image[index++] = height;
			}
		}
	}
}

void heightmap_image(struct byteimage *image, long seed, float frequency, float perturb)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::SimplexFractal);
	noise.SetFractalType(FastNoise::FBM);
	noise.SetFrequency(frequency);
	noise.SetFractalOctaves(5);
	noise.SetGradientPerturbAmp(perturb);

	const glm::vec2 center = glm::vec2(0.5f*float(image->width), 0.5f*float(image->height));
	unsigned int index = 0;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float x = i; float y = j;
			noise.GradientPerturbFractal(x, y);

			float height = (noise.GetNoise(x, y) + 1.f) / 2.f;
			height = glm::clamp(height, 0.f, 1.f);

			float mask = glm::distance(0.5f*float(image->width), float(y)) / (0.5f*float(image->width));
			mask = 1.f - glm::clamp(mask, 0.f, 1.f);
			mask = glm::smoothstep(0.2f, 0.5f, sqrtf(mask));
			height *= sqrtf(mask);

			image->data[index++] = 255.f * height;
		}
	}
}

void gradient_image(struct byteimage *image, long seed, float perturb)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	FastNoise noise;
	noise.SetSeed(seed);
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetFrequency(0.005);
	noise.SetGradientPerturbAmp(perturb);

	const float height = float(image->height);
	unsigned int index = 0;
	for (int i = 0; i < image->width; i++) {
		for (int j = 0; j < image->height; j++) {
			float y = i; float x = j;
			noise.GradientPerturbFractal(x, y);
			float lattitude = 1.f - (y / height);
			image->data[index++] = 255.f * glm::clamp(lattitude, 0.f, 1.f);
		}
	}
}

// http://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
static inline int orient(float x0, float y0, float x1, float y1, float x2, float y2)
{
	return ((int)x1 - (int)x0)*((int)y2 - (int)y0) - ((int)y1 - (int)y0)*((int)x2 - (int)x0);
}

static inline int min3(int a, int b, int c)
{
	return std::min(a, std::min(b, c));
}

static inline int max3(int a, int b, int c)
{
	return std::max(a, std::max(b, c));
}

void plot(int x, int y, unsigned char *image, int width, int height, int nchannels, unsigned char *color)
{
	if (x < 0 || y < 0 || x > (width-1) || y > (height-1)) {
		return;
	}

	int index = y * width * nchannels + x * nchannels;

	for (int i = 0; i < nchannels; i++) {
		image[index+i] = color[i];
	}
}

void draw_triangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned char *image, int width, int height, int nchannel, unsigned char *color)
{
	int area = orient(x0, y0, x1, y1, x2, y2);
	if (area == 0) { return; }

	// Compute triangle bounding box
	int minX = min3((int)x0, (int)x1, (int)x2);
	int minY = min3((int)y0, (int)y1, (int)y2);
	int maxX = max3((int)x0, (int)x1, (int)x2);
	int maxY = max3((int)y0, (int)y1, (int)y2);

	// Clip against screen bounds
	minX = std::max(minX, 0);
	minY = std::max(minY, 0);
	maxX = std::min(maxX, width - 1);
	maxY = std::min(maxY, height - 1);

	// Rasterize
	float px, py;
	for (py = minY; py <= maxY; py++) {
		for (px = minX; px <= maxX; px++) {
			// Determine barycentric coordinates
			int w0 = orient(x1, y1, x2, y2, px, py);
			int w1 = orient(x2, y2, x0, y0, px, py);
			int w2 = orient(x0, y0, x1, y1, px, py);

			// If p is on or inside all edges, render pixel.
			if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
				plot((int)px, (int)py, image, width, height, nchannel, color);
			}
		}
	}
}

void relax_points(const jcv_diagram* diagram, std::vector<jcv_point> &points)
{
	const jcv_site* sites = jcv_diagram_get_sites(diagram);
	for( int i = 0; i < diagram->numsites; ++i ) {
		const jcv_site* site = &sites[i];
		jcv_point sum = site->p;
		int count = 1;

		const jcv_graphedge* edge = site->edges;

		while (edge) {
			sum.x += edge->pos[0].x;
			sum.y += edge->pos[0].y;
			++count;
			edge = edge->next;
		}

		jcv_point point;
		point.x = sum.x / count;
		point.y = sum.y / count;
		points.push_back(point);
	}
}

#define SEA_LEVEL 0.5
#define MOUNTAIN_LEVEL 0.7
#define NSITES 256*256
void do_voronoi(struct byteimage *image, const struct byteimage *heightimage)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	std::vector<jcv_point> points;

	const float ratio = float(image->width) / float(heightimage->width);
	for (int i = 0; i < NSITES; i++) {
		jcv_point point;
		point.x = dis(gen);
		point.y = dis(gen);
		int x = point.x / ratio;
		int y = point.y / ratio;
		float height = sample_byte_height(x, y, heightimage);
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			height = 1.f;
		} else if (height < SEA_LEVEL) {
			height = 0.05f;
		} else {
			height = 0.25f;
		}
		std::bernoulli_distribution d(height);
		if (d(gen) == true) {
			points.push_back(point);
		}
	}

	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), 0, 0, &diagram);

	std::vector<jcv_point> relaxed_points;
	relax_points(&diagram, relaxed_points);

	jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), 0, 0, &diagram);

	// fill the cells
	const jcv_site *cells = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		unsigned char rcolor[3];
		unsigned char basecolor = 255;
		rcolor[0] = basecolor;
		rcolor[1] = basecolor;
		rcolor[2] = basecolor;
		const jcv_site *site = &cells[i];
		const jcv_graphedge *e = site->edges;
		const jcv_point p = site->p;
		int x = p.x / ratio;
		int y = p.y / ratio;
		float height = sample_byte_height(x, y, heightimage);
		if (height > SEA_LEVEL && height < MOUNTAIN_LEVEL) {
			height = 0.8f;
		} else if (height < SEA_LEVEL) {
			height = 0.05f;
		} else {
			height = 0.95f;
		}
		rcolor[0] *= height;
		rcolor[1] *= height;
		rcolor[2] *= height;

		while (e) {
			draw_triangle(site->p.x, site->p.y, e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, image->data, image->width, image->height, image->nchannels, rcolor);
			e = e->next;
		}
	}

	jcv_diagram_free(&diagram);
}
