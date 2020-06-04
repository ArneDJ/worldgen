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

void terrain_image(float *image, size_t sidelength, long seed, float freq, float mountain_amp, float field_amp);

struct floatimage gen_normalmap(const struct floatimage *heightmap);

float sample_image(int x, int y, const struct floatimage *image, unsigned int channel);

float sample_byte_height(int x, int y, const struct byteimage *image);

void billow_3D_image(unsigned char *image, size_t sidelength, float frequency, float cloud_distance);

void heightmap_image(struct byteimage *image, long seed, float frequency, float perturb);

void gradient_image(struct byteimage *image, long seed, float perturb);

void gauss_blur_image(struct byteimage *image, float sigma);

void draw_triangle(float x0, float y0, float x1, float y1, float x2, float y2, unsigned char *image, int width, int height, int nchannel, unsigned char *color);

void plot(int x, int y, unsigned char *image, int width, int height, int nchannels, unsigned char *color);

void draw_line(int x0, int y0, int x1, int y1, unsigned char *image, int width, int height, int nchannels, unsigned char *color);
