struct worldparams {
	float frequency;
	float perturbfreq;
	float perturbamp;
	unsigned int octaves;
	float lacunarity;
	// temperatures
	float tempfreq;
	float tempperturb;
	// rain
	float rainperturb;
	// elevation
	float lowland;
	float upland;
	float highland;
};

class Terraform {
public:
	struct floatimage heightmap;
	struct floatimage tempmap;
	struct byteimage rainmap;
public:
	Terraform(size_t imageres, long seed, struct worldparams params);
	~Terraform(void);
};
