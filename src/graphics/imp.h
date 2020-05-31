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

void billow_3D_image(unsigned char *image, size_t sidelength, float frequency, float cloud_distance);

void heightmap_image(struct byteimage *image, long seed, float frequency, float perturb);

void do_voronoi(struct byteimage *image, const struct byteimage *heightimage);

void gradient_image(struct byteimage *image, long seed, float perturb);
