class Terraform {
public:
	struct floatimage heightmap;
	struct byteimage temperature;
	struct byteimage rainfall;
public:
	Terraform(size_t width, size_t height, long seed, float freq);
	~Terraform(void);
};
