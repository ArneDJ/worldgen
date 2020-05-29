enum {
	FOURCC_DXT1 = 0x31545844, 
	FOURCC_DXT3 = 0x33545844, 
	FOURCC_DXT5 = 0x35545844,
};

// DDS header
struct DDS {
	char identifier[4]; /* file type */
	uint32_t height; /* in pixels */
	uint32_t width; /* in pixels */
	uint32_t linear_size;
	uint32_t mip_levels;
	uint32_t dxt_codec; /* compression type */
};

GLuint load_DDS_texture(const char *fpath);
unsigned char *load_DDS(const char *fpath, struct DDS *header);
