#ifndef GRAPHICSPRAKTIKUM_BUFFERIMAGE_H
#define GRAPHICSPRAKTIKUM_BUFFERIMAGE_H

#include <vulkan/vulkan_core.h>

typedef struct {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
} BufferImage;


#endif //GRAPHICSPRAKTIKUM_BUFFERIMAGE_H
