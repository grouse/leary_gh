/**
 * file:    assets.cpp
 * created: 2017-04-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct Texture {
	// TODO(jesper): internal non-Vulkan format?
	VkFormat format;
	u32      width;
	u32      height;
	isize    size;
	void     *data;
};

// NOTE(jesper): only Microsoft BMP version 3 is supported
PACKED(struct BitmapFileHeader {
	u16 type;
	u32 size;
	u16 reserved0;
	u16 reserved1;
	u32 offset;
});

PACKED(struct BitmapHeader {
	u32 header_size;
	i32 width;
	i32 height;
	u16 planes;
	u16 bpp;
	u32 compression;
	u32 bmp_size;
	i32 res_horiz;
	i32 res_vert;
	u32 colors_used;
	u32 colors_important;
});

Texture texture_load_r16(const char *filename, u32 width, u32 height)
{
	Texture texture = {};

	usize size;
	char *path = platform_resolve_path(GamePath_textures, filename);
	defer { free(path); };

	char *file = platform_file_read(path, &size);
	if (!file) {
		return texture;
	}

	texture.width  = width;
	texture.height = height;
	texture.size   = size;
	texture.data   = file;
	texture.format = VK_FORMAT_R16_UNORM;

	return texture;
}

Texture texture_load_bmp(const char *filename)
{
	Texture texture = {};

	usize size;
	char *path = platform_resolve_path(GamePath_textures, filename);
	char *file = platform_file_read(path, &size);

	defer {
		free(path);
		free(file);
	};

	DEBUG_ASSERT(file[0] == 'B' && file[1] == 'M');

	char *ptr = file;
	BitmapFileHeader *fh = (BitmapFileHeader*)ptr;

	if (fh->type != 0x4d42) {
		// TODO(jesper): support other bmp versions
		DEBUG_UNIMPLEMENTED();
		return Texture{};
	}

	ptr += sizeof(BitmapFileHeader);

	BitmapHeader *h = (BitmapHeader*)ptr;

	if (h->header_size != 40) {
		// TODO(jesper): support other bmp versions
		DEBUG_UNIMPLEMENTED();
		return Texture{};
	}

	DEBUG_ASSERT(h->header_size == 40);
	ptr += sizeof(BitmapHeader);

	if (h->compression != 0) {
		// TODO(jesper): support compression
		DEBUG_UNIMPLEMENTED();
		return Texture{};
	}

	if (h->colors_used == 0 && h->bpp < 16) {
		h->colors_used = 1 << h->bpp;
	}

	if (h->colors_important == 0) {
		h->colors_important = h->colors_used;
	}

	bool flip = true;
	if (h->height < 0) {
		flip = false;
		h->height = -h->height;
	}

	// NOTE(jesper): bmp's with bbp > 16 doesn't have a color palette
	// NOTE(jesper): and other bbps are untested atm
	if (h->bpp != 24) {
		// TODO(jesper): only 24 bpp is tested and supported
		DEBUG_UNIMPLEMENTED();
		return Texture{};
	}

	u8 channels = 4;

	texture.format = VK_FORMAT_B8G8R8A8_UNORM;
	texture.width  = h->width;
	texture.height = h->height;
	texture.size   = h->width * h->height * channels;
	texture.data   = malloc(texture.size);

	bool alpha = false;

	u8 *src = (u8*)ptr;
	u8 *dst = (u8*)texture.data;

	for (i32 i = 0; i < h->height; i++) {
		for (i32 j = 0; j < h->width; j++) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;

			if (alpha) {
				*dst++ = *src++;
			} else {
				*dst++ = 255;
			}
		}
	}

	// NOTE(jesper): flip bottom-up textures
	if (flip) {
		dst = (u8*)texture.data;
		for (i32 i = 0; i < h->height >> 2; i++) {
			u8 *p1 = &dst[i * h->width * channels];
			u8 *p2 = &dst[(h->height - 1 - i) * h->width * channels];
			for (i32 j = 0; j < h->width * channels; j++) {
				u8 tmp = p1[j];
				p1[j] = p2[j];
				p2[j] = tmp;
			}
		}
	}

	return texture;
}
