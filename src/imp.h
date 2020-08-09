/* 
 * imp - image manipulation program
 */
enum channel {
	RED = 0,
	GREEN = 1,
	BLUE = 2,
	ALPHA = 3
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

struct byteimage blank_byteimage(unsigned int nchannels, size_t width, size_t height);

void delete_byteimage(const byteimage *image);

void delete_floatimage(const floatimage *image);

int RGB_to_int(unsigned char r, unsigned char g, unsigned char b);

void plot(int x, int y, unsigned char *image, int width, int height, int nchannels, unsigned char *color);
void draw_line(int x0, int y0, int x1, int y1, unsigned char *image, int width, int height, int nchannels, unsigned char *color);
void draw_triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, unsigned char *image, int width, int height, int nchannel, unsigned char *color);

float sample_byteimage(int x, int y, enum channel chan, const struct byteimage *image);

int floodfill(int *image, size_t w, size_t h, int x, int y, int newcolor, int oldcolor);

void gauss_blur_image(struct byteimage *image, float sigma);

float sample_floatimage(int x, int y, enum channel chan, const struct floatimage *image);

struct floatimage gen_normalmap(const struct floatimage *heightmap);
