enum {
	RED_CHANNEL = 1,
	RG_CHANNEL = 2,
	RGB_CHANNEL = 3,
	RGBA_CHANNEL = 4
};

struct floatimage {
	float *data = nullptr;
	unsigned int nchannels;
	size_t width;
	size_t height;
};

struct byteimage {
	unsigned char *data = nullptr;
	unsigned int nchannels;
	size_t width;
	size_t height;
};

struct floatimage gen_normalmap(const struct floatimage *heightmap);

float sample_floatimage(int x, int y, unsigned int channel, const struct floatimage *image);

float sample_byteimage(int x, int y, unsigned int channel, const struct byteimage *image);

void gauss_blur_image(struct byteimage *image, float sigma);

void draw_triangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned char *image, int width, int height, int nchannel, unsigned char *color);

void plot(int x, int y, unsigned char *image, int width, int height, int nchannels, unsigned char *color);

void draw_line(int x0, int y0, int x1, int y1, unsigned char *image, int width, int height, int nchannels, unsigned char *color);
