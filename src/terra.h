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
	float tempinfluence;
	float rainblur;
	// elevation
	float lowland;
	float upland;
	float highland;
};

struct terraform {
	struct byteimage heightmap;
	struct byteimage tempmap;
	struct byteimage rainmap;
};

struct terraform form_terra(size_t imageres, long seed, struct worldparams params);
