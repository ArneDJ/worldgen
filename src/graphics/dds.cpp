#include <cstdlib>
#include <cstring>
#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>

#include "dds.h"

unsigned char *load_DDS(const char *fpath, struct DDS *header)
{
	FILE *fp = fopen(fpath, "rb");
	if (fp == nullptr) {
		perror(fpath);
		return nullptr;
	}

	/* verify the type of file */
	fread(header->identifier, 1, 4, fp);
	if (strncmp(header->identifier, "DDS ", 4) != 0) {
		std::cerr << "error: " << fpath << "not a valid DDS file\n";
		fclose(fp);
		return nullptr;
	}

	/* the header is 128 bytes, 124 after reading the file type */
	unsigned char header_buf[124];
	fread(&header_buf, 124, 1, fp);
	header->height = *(uint32_t*)&(header_buf[8]);
	header->width = *(uint32_t*)&(header_buf[12]);
	header->linear_size = *(uint32_t*)&(header_buf[16]);
	header->mip_levels = *(uint32_t*)&(header_buf[24]);
	header->dxt_codec = *(uint32_t*)&(header_buf[80]);

	/* now get the actual image data */
	unsigned char *image;
	uint32_t len = (header->mip_levels > 1) ? (header->linear_size * 2) : header->linear_size;
	image = new unsigned char[len];
	fread(image, 1, len, fp);

	fclose(fp);

	return image;
}

GLuint load_DDS_texture(const char *fpath)
{
	struct DDS header;
	unsigned char *image = load_DDS(fpath, &header);
	if (image == nullptr) { return 0; }

	/* find valid DXT format*/
	uint32_t components = (header.dxt_codec == FOURCC_DXT1) ? 3 : 4;
	uint32_t format;
	uint32_t block_size;
	switch (header.dxt_codec) {
	case FOURCC_DXT1: format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; block_size = 8; break;
	case FOURCC_DXT3: format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; block_size = 16; break;
	case FOURCC_DXT5: format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; block_size = 16; break;
	default:
		std::cerr << "error: no valid DXT format found for " << fpath << std::endl;
		delete [] image;
		return 0;
	};

	/* now make the opengl texture */
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, header.mip_levels-1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	unsigned int offset = 0;
	for (unsigned int i = 0; i < header.mip_levels; i++) {
		if (header.width <= 4 || header.height <= 4) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, i-1);
			break;
		}
		/* now to actually get the compressed image into opengl */
		unsigned int size = ((header.width+3)/4) * ((header.height+3)/4) * block_size;
		glCompressedTexImage2D(GL_TEXTURE_2D, i, format,
		header.width, header.height, 0, size, image + offset);

		offset += size;
		header.width = header.width/2;
		header.height = header.height/2;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	delete [] image;

	return texture;
}
