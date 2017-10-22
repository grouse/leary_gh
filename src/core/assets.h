/**
 * file:    assets.h
 * created: 2017-08-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_ASSETS_H
#define LEARY_ASSETS_H

#define ASSET_INVALID_ID (-1)

struct Texture {
    i32   id = ASSET_INVALID_ID;
    u32   width;
    u32   height;

    // TODO(jesper): we might not want these in this structure? I think the only
    // place that really needs it untemporarily is the heightmap, the other
    // places we'll just be uploading straight to the GPU and no longer require
    // the data
    isize size;
    void  *data;

    VkFormat       format;
    VkImage        image;
    VkImageView    image_view;
    VkDeviceMemory memory;
};


#endif /* LEARY_ASSETS_H */

