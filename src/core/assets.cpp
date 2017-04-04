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
	u32 width;
	u32 height;
	isize size;
	u8 *pixels;
};

// NOTE(jesper): only Microsoft BMP version 3 is supported
struct BitmapFileHeader {
	u16 type;
	u32 size;
	u16 reserved0;
	u16 reserved1;
	u32 offset;
} __attribute__((packed));

struct BitmapHeader {
	u32 header_size;
	i32 width;
	i32 height;
	u16 planes;
	u16 bbp;
	u32 compression;
	u32 bmp_size;
	i32 horz_resolution;
	i32 vert_resolution;
	u32 colors_used;
	u32 colors_important;
} __attribute__((packed));

Texture load_texture_bmp(const char *filename)
{
	Texture texture = {};

	usize size;
	char *path = platform_resolve_path(GamePath_textures, filename);
	char *file = platform_file_read(path, &size);

	DEBUG_ASSERT(file[0] == 'B' && file[1] == 'M');

	char *ptr = file;
	BitmapFileHeader *fh = (BitmapFileHeader*)ptr;
	// TODO(jesper): support other bmp versions
	DEBUG_ASSERT(fh->type == 0x4d42);
	ptr += sizeof(BitmapFileHeader);

	BitmapHeader *h = (BitmapHeader*)ptr;
	// TODO(jesper): use header_size to determine whether it's a BMP version 3
	// or version 2
	DEBUG_ASSERT(h->header_size == sizeof(BitmapHeader));
	DEBUG_ASSERT(h->header_size == 40);
	ptr += sizeof(BitmapHeader);

	// TODO(jesper): support compression
	DEBUG_ASSERT(h->compression == 0);

	if (h->colors_used == 0 && h->bbp < 16) {
		h->colors_used = 1 << h->bbp;
	}

	if (h->colors_important == 0) {
		h->colors_important = h->colors_used;
	}

	// NOTE(jesper): bmp's with bbp > 16 doesn't have a color palette
	// NOTE(jesper): and other bbps are untested atm
	DEBUG_ASSERT(h->bbp > 16);
	DEBUG_ASSERT(h->bbp == 24);

	u8 num_channels = (h->bbp / 3) + 1;

	texture.format = VK_FORMAT_R8G8B8A8_UNORM;
	texture.width  = h->width;
	texture.height = h->height;
	texture.size   = h->width * h->height * num_channels;
	texture.pixels = (u8*)malloc(texture.size);

	bool alpha = false;

	u8 *src = (u8*)ptr;
	u8 *dst = (u8*)texture.pixels;

	for (i32 i = 0; i < h->width; i++) {
		for (i32 j = 0; j < h->height; j++) {
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

#if 0
	if (h->height > 0) { // bottom-up
	} else { // top-down
		DEBUG_ASSERT(h->height < 0);
	}
#endif

	return texture;
}

