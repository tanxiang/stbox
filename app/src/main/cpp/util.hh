//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_UTIL_H
#define STBOX_UTIL_H

#include <unistd.h>
#include <android/log.h>
#include "vulkan.hpp"
#include "main.hh"
#include "glm/glm.hpp"

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

namespace tt {
    class Device : public vk::Device {
        vk::PhysicalDevice &physicalDevice;
        uint32_t queueFamilyIndex;
        vk::UniqueCommandPool commandPool;
        vk::UniqueSwapchainKHR swapchainKHR;
        vk::Extent2D swapchainExtent;
        vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
        std::vector<std::pair<vk::Image, vk::UniqueImageView>> vkSwapChainBuffers;
        vk::UniqueImage depthImage;
        vk::UniqueImageView depthImageView;
        vk::UniqueDeviceMemory depthImageMemory;
        vk::UniqueBuffer mvpBuffer;
        vk::UniqueDeviceMemory mvpMemory;

        std::array<vk::DescriptorSetLayoutBinding, 2> descriptSlBs{
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
        };
        vk::UniqueDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayoutUnique(
                vk::DescriptorSetLayoutCreateInfo{
            vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(), descriptSlBs.data()
        });
        auto pipelineLayoutUnique = createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{
                vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0, nullptr
        });
        uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);
        vk::DescriptorPoolSize poolSize[2]{{
                                                   vk::DescriptorType::eUniformBuffer,        1
                                           },
                                           {
                                                   vk::DescriptorType::eCombinedImageSampler, 1
                                           }};
        auto descriptorPoll = createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                vk::DescriptorPoolCreateFlags(), 1, 2, poolSize});
        auto descriptorSets = allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo{
                descriptorPoll.get(), 1, &descriptorSetLayout.get()
        });
        vk::UniqueRenderPass renderPass;
        std::vector<vk::UniqueFramebuffer> frameBuffers;
    public:
        Device(vk::Device dev, vk::PhysicalDevice &phy, uint32_t qidx) : vk::Device{
                dev}, physicalDevice{phy}, queueFamilyIndex{qidx}, commandPool{
                createCommandPoolUnique(vk::CommandPoolCreateInfo{
                        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qidx})} {

        }
        ~Device(){
            waitIdle();
            destroy();
        }
        vk::Extent2D getSwapchainExtent(){
            return swapchainExtent;
        }
        std::vector<vk::CommandBuffer> defaultPoolAllocBuffer(vk::CommandBufferLevel bufferLevel,uint32_t num);
        vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

        vk::UniqueDeviceMemory AllocBindImageMemory(vk::Image image,vk::MemoryPropertyFlags flags);
        void buildSwapchainViewBuffers(vk::SurfaceKHR &surfaceKHR);
        void buildMVPBufferAndWrite(glm::mat4 MVP);
        void buildRenderpass(vk::SurfaceKHR &surfaceKHR);
    };

    class Instance : public vk::Instance {
        vk::SurfaceKHR surfaceKHR = createAndroidSurfaceKHR(
                vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
                                                AndroidGetApplicationWindow()});
        std::vector<vk::PhysicalDevice> vkPhysicalDevices = enumeratePhysicalDevices();
    public:
        Instance(vk::Instance ins) : vk::Instance{ins} {

        }

        vk::PhysicalDevice &defaultPhyDevice() {
            return vkPhysicalDevices[0];
        }

        vk::SurfaceKHR &defaultSurface() {
            return surfaceKHR;
        }

        uint32_t queueFamilyPropertiesFindFlags(vk::QueueFlags);

        tt::Device connectToDevice();

        ~Instance(){
            destroySurfaceKHR(surfaceKHR);
            destroy();
        }

    };

    Instance createInstance();
}
#endif //STBOX_UTIL_H
