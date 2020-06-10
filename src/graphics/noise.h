
void billow_3D_image(unsigned char *image, size_t sidelength, float frequency, float cloud_distance);

void cellnoise_image(float *image, size_t sidelength, long seed, float freq);
void badlands_image(float *image, size_t sidelength, long seed, float freq);

void heightmap_image(struct byteimage *image, long seed, float frequency, float perturb);

void simplex_image(struct floatimage *image, long seed, float frequency, float perturb);

void gradient_image(struct byteimage *image, long seed, float perturb);
