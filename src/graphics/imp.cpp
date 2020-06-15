#include <iostream>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define IIR_GAUSS_BLUR_IMPLEMENTATION
#include "../external/gauss/iir_gauss_blur.h"

#include "imp.h"

static inline float sample_height(int x, int y, const struct floatimage *image)
{
	if (x < 0 || y < 0 || x > (image->width-1) || y > (image->height-1)) {
		return 0.f; 
	}

	int index = y * image->width * image->nchannels + x * image->nchannels;

	return image->data[index];
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

float sample_byteimage(int x, int y, unsigned int channel, const struct byteimage *image)
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

	return image->data[index+channel] / 255.f;
}


float sample_floatimage(int x, int y, unsigned int channel, const struct floatimage *image)
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
		}
	}

	return normalmap;
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

// http://members.chello.at/~easyfilter/bresenham.html
void draw_line(int x0, int y0, int x1, int y1, unsigned char *image, int width, int height, int nchannels, unsigned char *color)
{
	int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = dx+dy, e2; // error value e_xy

	for(;;) {
		plot(x0,y0, image, width, height, nchannels, color);
		if (x0==x1 && y0==y1) { break; }
		e2 = 2*err;
		if (e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
		if (e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
	}
}

void draw_bezier_segment(int x0, int y0, int x1, int y1, int x2, int y2, struct byteimage *image, unsigned char *color)
{
	int sx = x2-x1, sy = y2-y1;
	long xx = x0-x1, yy = y0-y1, xy;         /* relative values for checks */
	double dx, dy, err, cur = xx*sy-yy*sx;                    /* curvature */

	if (xx*sx <= 0 && yy*sy <= 0) {  /* sign of gradient must not change */
		//return;
	}

	if (sx*(long)sx+sy*(long)sy > xx*xx+yy*yy) { /* begin with longer part */
	x2 = x0; x0 = sx+x1; y2 = y0; y0 = sy+y1; cur = -cur;  /* swap P0 P2 */
	}
	if (cur != 0) {                                    /* no straight line */
	xx += sx; xx *= sx = x0 < x2 ? 1 : -1;           /* x step direction */
	yy += sy; yy *= sy = y0 < y2 ? 1 : -1;           /* y step direction */
	xy = 2*xx*yy; xx *= xx; yy *= yy;          /* differences 2nd degree */
	if (cur*sx*sy < 0) {                           /* negated curvature? */
	xx = -xx; yy = -yy; xy = -xy; cur = -cur;
	}
	dx = 4.0*sy*cur*(x1-x0)+xx-xy;             /* differences 1st degree */
	dy = 4.0*sx*cur*(y0-y1)+yy-xy;
	xx += xx; yy += yy; err = dx+dy+xy;                /* error 1st step */
	do {
	//setPixel(x0,y0);                                     /* plot curve */
	plot(x0,y0, image->data, image->width, image->height, image->nchannels, color);
	if (x0 == x2 && y0 == y2) return;  /* last pixel -> curve finished */
	y1 = 2*err < dx;                  /* save value for test of y step */
	if (2*err > dy) { x0 += sx; dx -= xy; err += dy += yy; } /* x step */
	if (    y1    ) { y0 += sy; dy -= xy; err += dx += xx; } /* y step */
	} while (dy < dx );           /* gradient negates -> algorithm fails */
	}
	//plotLine(x0,y0, x2,y2);                  /* plot remaining part to end */
	draw_line(x0, y0, x2, y2, image->data, image->width, image->height, image->nchannels, color);
}

void draw_bezier(int x0, int y0, int x1, int y1, int x2, int y2, struct byteimage *image, unsigned char *color)
{ /* plot any quadratic Bezier curve */
 int x = x0-x1, y = y0-y1;
 double t = x0-2*x1+x2, r;
 if ((long)x*(x2-x1) > 0) { /* horizontal cut at P4? */
 if ((long)y*(y2-y1) > 0) /* vertical cut at P6 too? */
 if (fabs((y0-2*y1+y2)/t*x) > abs(y)) { /* which first? */
 x0 = x2; x2 = x+x1; y0 = y2; y2 = y+y1; /* swap points */
 } /* now horizontal cut at P4 comes first */
 t = (x0-x1)/t;
 r = (1-t)*((1-t)*y0+2.0*t*y1)+t*t*y2; /* By(t=P4) */
 t = (x0*x2-x1*x1)*t/(x0-x1); /* gradient dP4/dx=0 */
 x = floor(t+0.5); y = floor(r+0.5);
 r = (y1-y0)*(t-x0)/(x1-x0)+y0; /* intersect P3 | P0 P1 */
 draw_bezier_segment(x0,y0, x,floor(r+0.5), x,y, image, color);
 r = (y1-y2)*(t-x2)/(x1-x2)+y2; /* intersect P4 | P1 P2 */
 x0 = x1 = x; y0 = y; y1 = floor(r+0.5); /* P0 = P4, P1 = P8 */
 }
 if ((long)(y0-y1)*(y2-y1) > 0) { /* vertical cut at P6? */
 t = y0-2*y1+y2; t = (y0-y1)/t;
 r = (1-t)*((1-t)*x0+2.0*t*x1)+t*t*x2; /* Bx(t=P6) */
 t = (y0*y2-y1*y1)*t/(y0-y1); /* gradient dP6/dy=0 */
 x = floor(r+0.5); y = floor(t+0.5);
 r = (x1-x0)*(t-y0)/(y1-y0)+x0; /* intersect P6 | P0 P1 */
 draw_bezier_segment(x0,y0, floor(r+0.5),y, x,y, image, color);
 r = (x1-x2)*(t-y2)/(y1-y2)+x2; /* intersect P7 | P1 P2 */
 x0 = x; x1 = floor(r+0.5); y0 = y1 = y; /* P0 = P6, P1 = P7 */
  }
 draw_bezier_segment(x0,y0, x1,y1, x2,y2, image, color); /* remaining part */
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

void gauss_blur_image(struct byteimage *image, float sigma)
{
	if (image->data == nullptr) {
		std::cerr << "no memory present\n";
		return;
	}

	iir_gauss_blur(image->width, image->height, image->nchannels, image->data, sigma);
}
