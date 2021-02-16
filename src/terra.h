struct worldparams {
	// heightmap
	float frequency;
	float perturbfreq;
	float perturbamp;
	unsigned int octaves;
	float lacunarity;
	// temperatures
	float tempfreq;
	float tempperturb;
	// rain
	float rainblur;
	// relief
	float lowland;
	float upland;
	float highland;
	bool erodmountains;
};

struct terraform {
	struct byteimage heightmap;
	struct byteimage tempmap;
	struct byteimage rainmap;
};

struct terraform form_terra(size_t imageres, long seed, struct worldparams params);
