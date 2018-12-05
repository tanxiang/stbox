//
// Created by ttand on 18-2-12.
//
#include "vulkan.hpp"

#include "stboxvk.hh"
#include <iostream>
//#include <cassert>
#include "main.hh"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

#include "util.hh"
#include "vertexdata.hh"

#if 0
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		assert(res == VK_SUCCESS);																		\
	}																									\
}

android_app* androidApp;

VkPhysicalDevice physicalDevice;
VkDevice device;
VkPhysicalDeviceMemoryProperties memoryProperties;
VkCommandPool cmdPool;
VkQueue queue;
struct Texture {
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView view;
    uint32_t width, height;
    uint32_t mipLevels;
} texture;

inline VkMemoryAllocateInfo memoryAllocateInfo()
{
    VkMemoryAllocateInfo memAllocInfo {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    return memAllocInfo;
}
inline VkBufferCreateInfo mmbufferCreateInfo()
{
    VkBufferCreateInfo bufCreateInfo {};
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    return bufCreateInfo;
}
inline VkImageCreateInfo mmimageCreateInfo()
{
    VkImageCreateInfo imageCreateInfo {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    return imageCreateInfo;
}

uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    else
    {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

inline VkCommandBufferAllocateInfo mmcommandBufferAllocateInfo(
        VkCommandPool commandPool,
        VkCommandBufferLevel level,
        uint32_t bufferCount)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = level;
    commandBufferAllocateInfo.commandBufferCount = bufferCount;
    return commandBufferAllocateInfo;
}
inline VkCommandBufferBeginInfo mmcommandBufferBeginInfo()
{
    VkCommandBufferBeginInfo cmdBufferBeginInfo {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    return cmdBufferBeginInfo;
}

VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            mmcommandBufferAllocateInfo(
                    cmdPool,
                    level,
                    1);

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

    // If requested, also start the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = mmcommandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
}


void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK_RESULT(vkQueueWaitIdle(queue));

    if (free)
    {
        vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
    }
}

inline VkImageMemoryBarrier mmimageMemoryBarrier()
{
    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    return imageMemoryBarrier;
}

inline VkSamplerCreateInfo mmsamplerCreateInfo()
{
    VkSamplerCreateInfo samplerCreateInfo {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    return samplerCreateInfo;
}

inline VkImageViewCreateInfo imageViewCreateInfo()
{
    VkImageViewCreateInfo imageViewCreateInfo {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    return imageViewCreateInfo;
}

void loadTexture()
{
    // We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
    std::string filename = "textures/metalplate01_rgba.ktx";
    // Texture data contains 4 channels (RGBA) with unnormalized 8-bit values, this is the most commonly supported format
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

#if defined(__ANDROID__)
    // Textures are stored inside the apk on Android (compressed)
    // So they need to be loaded via the asset manager
    AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
    assert(asset);
    size_t size = AAsset_getLength(asset);
    assert(size > 0);

    void *textureData = malloc(size);
    AAsset_read(asset, textureData, size);
    AAsset_close(asset);

    gli::texture2d tex2D(gli::load((const char*)textureData, size));
#else
    gli::texture2d tex2D(gli::load(filename));
#endif

    assert(!tex2D.empty());

    texture.width = static_cast<uint32_t>(tex2D[0].extent().x);
    texture.height = static_cast<uint32_t>(tex2D[0].extent().y);
    texture.mipLevels = static_cast<uint32_t>(tex2D.levels());

    // We prefer using staging to copy the texture data to a device local optimal image
    VkBool32 useStaging = true;

    // Only use linear tiling if forced
    bool forceLinearTiling = false;
    if (forceLinearTiling) {
        // Don't use linear if format is not supported for (linear) shader sampling
        // Get device properites for the requested texture format
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
        useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    }

    VkMemoryAllocateInfo memAllocInfo = memoryAllocateInfo();
    VkMemoryRequirements memReqs = {};

    if (useStaging) {
        // Copy data to an optimal tiled image
        // This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

        // Create a host-visible staging buffer that contains the raw image data
        // This buffer will be the data source for copying texture data to the optimal tiled image on the device
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferCreateInfo = mmbufferCreateInfo();
        bufferCreateInfo.size = tex2D.size();
        // This buffer is used as a transfer source for the buffer copy
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

        // Get memory requirements for the staging buffer (alignment, memory type bits)
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        // Get memory type index for a host visible buffer
        memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

        // Copy texture data into host local staging buffer
        uint8_t *data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
        memcpy(data, tex2D.data(), tex2D.size());
        vkUnmapMemory(device, stagingMemory);

        // Setup buffer copy regions for each mip level
        std::vector<VkBufferImageCopy> bufferCopyRegions;
        uint32_t offset = 0;

        for (uint32_t i = 0; i < texture.mipLevels; i++) {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = i;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2D[i].extent().x);
            bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2D[i].extent().y);
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;

            bufferCopyRegions.push_back(bufferCopyRegion);

            offset += static_cast<uint32_t>(tex2D[i].size());
        }

        // Create optimal tiled target image on the device
        VkImageCreateInfo imageCreateInfo = mmimageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = texture.mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // Set initial layout of the image to undefined
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = { texture.width, texture.height, 1 };
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

        vkGetImageMemoryRequirements(device, texture.image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.deviceMemory, 0));

        VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        // Image memory barriers for the texture image

        // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
        VkImageSubresourceRange subresourceRange = {};
        // Image only contains color data
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Start at first mip level
        subresourceRange.baseMipLevel = 0;
        // We will transition on all mip levels
        subresourceRange.levelCount = texture.mipLevels;
        // The 2D texture only has one layer
        subresourceRange.layerCount = 1;

        // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
        VkImageMemoryBarrier imageMemoryBarrier = mmimageMemoryBarrier();;
        imageMemoryBarrier.image = texture.image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
        // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
        // Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
        vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer,
                texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(bufferCopyRegions.size()),
                bufferCopyRegions.data());

        // Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
        // Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
        // Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
        vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

        // Store current layout for later reuse
        texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        flushCommandBuffer(copyCmd, queue, true);

        // Clean up staging resources
        vkFreeMemory(device, stagingMemory, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
    } else {
        // Copy data to a linear tiled image

        VkImage mappableImage;
        VkDeviceMemory mappableMemory;

        // Load mip map level 0 to linear tiling image
        VkImageCreateInfo imageCreateInfo = mmimageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageCreateInfo.extent = { texture.width, texture.height, 1 };
        VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage));

        // Get memory requirements for this image like size and alignment
        vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
        // Set memory allocation size to required memory size
        memAllocInfo.allocationSize = memReqs.size;
        // Get memory type that can be mapped to host memory
        memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, mappableImage, mappableMemory, 0));

        // Map image memory
        void *data;
        VK_CHECK_RESULT(vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data));
        // Copy image data of the first mip level into memory
        memcpy(data, tex2D[0].data(), tex2D[0].size());
        vkUnmapMemory(device, mappableMemory);

        // Linear tiled images don't need to be staged and can be directly used as textures
        texture.image = mappableImage;
        texture.deviceMemory = mappableMemory;
        texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Setup image memory barrier transfer image to shader read layout
        VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        // The sub resource range describes the regions of the image we will be transition
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        // Transition the texture image layout to shader read, so it can be sampled from
        VkImageMemoryBarrier imageMemoryBarrier = mmimageMemoryBarrier();;
        imageMemoryBarrier.image = texture.image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
        // Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
        // Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
        vkCmdPipelineBarrier(
                copyCmd,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

        flushCommandBuffer(copyCmd, queue, true);
    }

    // Create a texture sampler
    // In Vulkan textures are accessed by samplers
    // This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
    // Note: Similar to the samplers available with OpenGL 3.3
    VkSamplerCreateInfo sampler = mmsamplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    // Set max level-of-detail to mip level count of the texture
    sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
    // Enable anisotropic filtering
    // This feature is optional, so we must check if it's supported on the device
    //if (vulkanDevice->features.samplerAnisotropy) {
        // Use max. level of anisotropy for this example
    //    sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
    //    sampler.anisotropyEnable = VK_TRUE;
    //} else {
        // The device does not support anisotropic filtering
        sampler.maxAnisotropy = 1.0;
        sampler.anisotropyEnable = VK_FALSE;
   // }
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

    // Create image view
    // Textures are not directly accessed by the shaders and
    // are abstracted by image views containing additional
    // information and sub resource ranges
    VkImageViewCreateInfo view = imageViewCreateInfo();
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    // The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
    // It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    // Linear tiling usually won't support mip maps
    // Only set mip map count if optimal tiling is used
    view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
    // The view will be based on the texture's image
    view.image = texture.image;
    VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));
}
// Free all Vulkan resources used by a texture object
void destroyTextureImage(Texture texture)
{
    return;
    vkDestroyImageView(device, texture.view, nullptr);
    vkDestroyImage(device, texture.image, nullptr);
    vkDestroySampler(device, texture.sampler, nullptr);
    vkFreeMemory(device, texture.deviceMemory, nullptr);
}
#endif

namespace tt {

    void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
        assert(instance);
        auto surface = instance.connectToWSI(app->window);
        //std::cout<<"instance.connectToWSI"<<std::endl;
        //auto phyDevs = instance->enumeratePhysicalDevices();
        //std::cout<<"instance.connectToDevice"<<std::endl
        if(!devicePtr || !devicePtr->checkSurfaceSupport(surface.get()))
            devicePtr = instance.connectToDevice(surface.get());//reconnect
        //std::cout<<"instance.connectToDevice"<<std::endl;
        swapchainPtr = std::make_unique<tt::Swapchain>(std::move(surface), *devicePtr);

        //std::cout<<"create Swapchain"<<std::endl;

        auto descriptorSetLayout = devicePtr->createDescriptorSetLayoutUnique(
                std::vector<vk::DescriptorSetLayoutBinding>{
                        vk::DescriptorSetLayoutBinding{
                                0,
                                vk::DescriptorType::eUniformBuffer,
                                1,
                                vk::ShaderStageFlagBits::eVertex
                        },
                        vk::DescriptorSetLayoutBinding{
                                1,
                                vk::DescriptorType::eCombinedImageSampler,
                                1,
                                vk::ShaderStageFlagBits::eFragment
                        }
                }
        );
        devicePtr->descriptorSets = devicePtr->createDescriptorSets(descriptorSetLayout);

        //static auto Model = glm::mat4{1.0f};


        auto swapchainExtent = swapchainPtr->getSwapchainExtent();

        static auto Projection =glm::perspective(glm::radians(60.0f), static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 256.0f);

        static auto View = glm::lookAt(
                glm::vec3(8, 3, 10),  // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),     // and looks at the origin
                glm::vec3(0, 1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
        );

        //glm::rotate(View, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //static auto Clip = glm::mat4{1.0f, 0.0f, 0.0f, 0.0f,
        //                             0.0f, 1.0f, 0.0f, 0.0f,
        //                             0.0f, 0.0f, 0.5f,  0.0f,
        //                             0.0f, 0.0f, 0.5f, 1.0f};

        static auto mvpMat4 =  Projection  * View ;
        //std::cout<<__func__<< static_cast<VkBuffer>(std::get<vk::UniqueBuffer>(mvpBuffer).get())<<std::endl;
        devicePtr->mvpBuffer = devicePtr->createBufferAndMemory(sizeof(glm::mat4),
                                                 vk::BufferUsageFlagBits::eUniformBuffer,
                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                 vk::MemoryPropertyFlagBits::eHostCoherent);
        //std::cout<<__func__<< static_cast<VkBuffer>(std::get<vk::UniqueBuffer>(mvpBuffer).get())<<std::endl;
        {
            auto mvpBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->mvpBuffer);
            //todo copy to buffer
            memcpy(mvpBuffer_ptr.get(), &mvpMat4, std::get<size_t>(devicePtr->mvpBuffer));
        }



        std::vector<VertexUV> vertices{
                {{1.0f,  2.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-1.0f, 3.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
        };

        /*std::vector<Vertex> vertices2{
                {1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f,1.0f,1.0f},
                {-1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,1.0f,1.0f},
                {-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,1.0f},
                {1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,1.0f,1.0f}
        };*/
        auto pipelineLayout = devicePtr->createPipelineLayout(descriptorSetLayout);
        devicePtr->graphicsPipeline = devicePtr->createPipeline(sizeof(decltype(vertices)::value_type), app, *swapchainPtr,
                                  pipelineLayout.get());
        devicePtr->vertexBuffer = devicePtr->createBufferAndMemory(
                sizeof(decltype(vertices)::value_type) * vertices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        {
            auto vertexBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->vertexBuffer);
            memcpy(vertexBuffer_ptr.get(),vertices.data(),std::get<size_t>(devicePtr->vertexBuffer));
        }
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        devicePtr->indexBuffer = devicePtr->createBufferAndMemory(
                sizeof(decltype(indices)::value_type) * indices.size(),
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        {
            auto indexBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->indexBuffer);
            memcpy(indexBuffer_ptr.get(), indices.data(), std::get<size_t>(devicePtr->indexBuffer));
        }

        gli::texture2d tex2d;
        {
            auto fileContent = loadDataFromAssets("textures/vulkan_11_rgba.ktx", app);
            tex2d = gli::texture2d{gli::load(fileContent.data(), fileContent.size())};
        }
        vk::ImageSubresourceRange imageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0, tex2d.levels(), 0, 1
        };
        auto sampleImage =devicePtr->createImageAndMemory(vk::Format::eR8G8B8A8Unorm,vk::Extent3D{static_cast<uint32_t>(tex2d[0].extent().x),static_cast<uint32_t>(tex2d[0].extent().y),1},
                                                          vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferDst,
                                                          tex2d.levels(),
                                                          vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
                                                          imageSubresourceRange);
        {
            auto sampleBuffer = devicePtr->createBufferAndMemory(
                    tex2d.size(),
                    vk::BufferUsageFlagBits::eTransferSrc,
                    vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent);
            {
                auto sampleBufferPtr = devicePtr->mapMemoryAndSize(sampleBuffer);
                memcpy(sampleBufferPtr.get(), tex2d.data(), tex2d.size());
            }

            vk::ImageMemoryBarrier imageMemoryBarrierToDest{
                    vk::AccessFlags{}, vk::AccessFlagBits::eTransferWrite,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 0, 0,
                    std::get<vk::UniqueImage>(sampleImage).get(),
                    imageSubresourceRange
            };
            std::vector<vk::BufferImageCopy> bufferCopyRegion;
            for (int i = 0, offset = 0; i < tex2d.levels(); ++i) {
                std::cout << "BufferImageCopy" << tex2d[i].extent().x << 'X' << tex2d[i].extent().y
                          << std::endl;
                bufferCopyRegion.emplace_back(offset, 0, 0,
                                              vk::ImageSubresourceLayers{
                                                      vk::ImageAspectFlagBits::eColor, i, 0, 1},
                                              vk::Offset3D{},
                                              vk::Extent3D{tex2d[i].extent().x, tex2d[i].extent().y,1});
                offset += tex2d[i].size();
            }

            auto copyCmd = devicePtr->createCmdBuffers(1,
                                                       [&](CommandBufferBeginHandle &commandBufferBeginHandle) {
                                                           commandBufferBeginHandle.pipelineBarrier(
                                                                   vk::PipelineStageFlagBits::eHost,
                                                                   vk::PipelineStageFlagBits::eTransfer,
                                                                   vk::DependencyFlags{},
                                                                   0, nullptr,
                                                                   0, nullptr,
                                                                   1, &imageMemoryBarrierToDest);

                                                           commandBufferBeginHandle.copyBufferToImage(
                                                                   std::get<vk::UniqueBuffer>(sampleBuffer).get(),
                                                                   std::get<vk::UniqueImage>(sampleImage).get(),
                                                                   vk::ImageLayout::eTransferDstOptimal,
                                                                   bufferCopyRegion);
                                                           vk::ImageMemoryBarrier imageMemoryBarrierToGeneral{
                                                                   vk::AccessFlagBits::eTransferWrite,
                                                                   vk::AccessFlagBits::eShaderRead,
                                                                   vk::ImageLayout::eTransferDstOptimal,
                                                                   vk::ImageLayout::eShaderReadOnlyOptimal, 0, 0,
                                                                   std::get<vk::UniqueImage>(
                                                                           sampleImage).get(),
                                                                   imageSubresourceRange
                                                           };
                                                           commandBufferBeginHandle.pipelineBarrier(
                                                                   vk::PipelineStageFlagBits::eTransfer,
                                                                   vk::PipelineStageFlagBits::eFragmentShader,
                                                                   vk::DependencyFlags{},
                                                                   0, nullptr,
                                                                   0, nullptr,
                                                                   1, &imageMemoryBarrierToGeneral);
                                                       });

            auto fence = (*devicePtr)->createFenceUnique(vk::FenceCreateInfo{});
            std::array<vk::SubmitInfo, 1> submitInfos{
                    vk::SubmitInfo{
                            0, nullptr, nullptr,
                            1, &copyCmd[0].get()
                    }
            };
            devicePtr->graphsQueue().submit(submitInfos, fence.get());
            (*devicePtr)->waitForFences(1,&fence.get(),1,100000000000);

        }


        vk::SamplerCreateInfo samplerCreateInfo{vk::SamplerCreateFlags(),
                                                vk::Filter::eLinear,vk::Filter::eLinear,
                                                vk::SamplerMipmapMode::eLinear,
                                                vk::SamplerAddressMode::eRepeat,
                                                vk::SamplerAddressMode::eRepeat,
                                                vk::SamplerAddressMode::eRepeat,
                                                0,
                                                devicePtr->phyDevice().getFeatures().samplerAnisotropy,
                                                devicePtr->phyDevice().getProperties().limits.maxSamplerAnisotropy,
                                                0,vk::CompareOp::eNever,0,tex2d.levels(),vk::BorderColor::eFloatOpaqueWhite,0
        };
        auto sampler = (*devicePtr)->createSamplerUnique(samplerCreateInfo);
        vk::DescriptorImageInfo descriptorImageInfo{
            sampler.get(),std::get<vk::UniqueImageView >(sampleImage).get(),vk::ImageLayout::eShaderReadOnlyOptimal
        };
        auto descriptorBufferInfo = devicePtr->mvpBuffer.getDescriptorBufferInfo();
#if 0
        {
            androidApp = app;
            physicalDevice = static_cast<VkPhysicalDevice>(devicePtr->phyDevice());
            device = static_cast<VkDevice>(devicePtr->get());
            memoryProperties = devicePtr->phyDevice().getMemoryProperties();
            cmdPool = static_cast<VkCommandPool>(devicePtr->commandPool.get());
            queue = static_cast<VkQueue>(devicePtr->graphsQueue());
            loadTexture();
        }
        vk::DescriptorImageInfo textureDescriptor;
        textureDescriptor.imageView = texture.view;				// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
        textureDescriptor.sampler = texture.sampler;			// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
        textureDescriptor.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);	// The current layout of the image (Note: Should always fit the actual use, e.g. shader read)
#endif

        std::array<vk::WriteDescriptorSet,2> writeDes{
                vk::WriteDescriptorSet{
                    devicePtr->descriptorSets[0].get(),0,0,1,vk::DescriptorType::eUniformBuffer,
                    nullptr,&descriptorBufferInfo
                },
                vk::WriteDescriptorSet{
                    devicePtr->descriptorSets[0].get(),1,0,1,vk::DescriptorType::eCombinedImageSampler,
                    &descriptorImageInfo
                }
        };
        (*devicePtr)->updateDescriptorSets(writeDes, nullptr);

        devicePtr->mianBuffers = devicePtr->createCmdBuffers(*swapchainPtr,[&](RenderpassBeginHandle& cmdHandleRenderpassBegin){
            vk::Viewport viewport{
                0, 0, swapchainExtent.width, swapchainExtent.height, 0.0f, 1.0f
            };
            cmdHandleRenderpassBegin.setViewport(0,1,&viewport);
            vk::Rect2D scissors{vk::Offset2D{}, swapchainExtent};
            cmdHandleRenderpassBegin.setScissor(0,1,&scissors);

            cmdHandleRenderpassBegin.bindPipeline(vk::PipelineBindPoint::eGraphics, devicePtr->graphicsPipeline.get());
            std::array<vk::DescriptorSet, 1> tmpDescriptorSets{
                devicePtr->descriptorSets[0].get()
            };
            cmdHandleRenderpassBegin.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0,
                                                        tmpDescriptorSets, std::vector<uint32_t>{});
            vk::DeviceSize offsets[1] = {0};
            cmdHandleRenderpassBegin.bindVertexBuffers(0, 1, &std::get<vk::UniqueBuffer>(devicePtr->vertexBuffer).get(), offsets);
            cmdHandleRenderpassBegin.bindIndexBuffer(std::get<vk::UniqueBuffer>(devicePtr->indexBuffer).get(),0,vk::IndexType::eUint32);
            cmdHandleRenderpassBegin.drawIndexed(indices.size(),1,0,0,0);
        });
        devicePtr->submitCmdBuffer(*swapchainPtr,devicePtr->mianBuffers);
    }

    void stboxvk::cleanWindow() {
        std::cout << __func__ << std::endl;
        swapchainPtr.reset();
        devicePtr.reset();
    }

}
