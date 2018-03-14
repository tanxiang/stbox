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
        std::vector<vk::DescriptorSetLayoutBinding> descriptSlBs{{
                                                                         0,
                                                                         vk::DescriptorType::eUniformBuffer,
                                                                         1,
                                                                         vk::ShaderStageFlagBits::eVertex
                                                                 },
                                                                 {
                                                                         1,
                                                                         vk::DescriptorType::eCombinedImageSampler,
                                                                         1,
                                                                         vk::ShaderStageFlagBits::eFragment
                                                                 }
        };
        vk::UniqueDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayoutUnique(
                vk::DescriptorSetLayoutCreateInfo{
                        vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(),
                        descriptSlBs.data()
                });
        vk::UniquePipelineLayout pipelineLayout = createPipelineLayoutUnique(
                vk::PipelineLayoutCreateInfo{
                        vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0, nullptr
                });
        std::vector<vk::DescriptorPoolSize> poolSize{{
                                                             vk::DescriptorType::eUniformBuffer,        1
                                                     },
                                                     {
                                                             vk::DescriptorType::eCombinedImageSampler, 1
                                                     }};
        vk::UniqueDescriptorPool descriptorPoll = createDescriptorPoolUnique(
                vk::DescriptorPoolCreateInfo{
                        vk::DescriptorPoolCreateFlags(), 1, poolSize.size(), poolSize.data()});
        std::vector<vk::UniqueDescriptorSet> descriptorSets = allocateDescriptorSetsUnique(
                vk::DescriptorSetAllocateInfo{
                        descriptorPoll.get(), 1, &descriptorSetLayout.get()
                });
        vk::UniqueRenderPass renderPass;
        std::vector<vk::UniqueFramebuffer> frameBuffers;
        vk::UniquePipelineCache vkPipelineCache = createPipelineCacheUnique(
                vk::PipelineCacheCreateInfo{});
        vk::UniquePipeline graphicsPipeline;

        uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);

    public:
        Device(vk::Device dev, vk::PhysicalDevice &phy, uint32_t qidx) : vk::Device{
                dev}, physicalDevice{phy}, queueFamilyIndex{qidx}, commandPool{
                createCommandPoolUnique(vk::CommandPoolCreateInfo{
                        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qidx})} {

        }

        Device(Device &&odevice) : physicalDevice{odevice.physicalDevice},
                                   queueFamilyIndex{odevice.queueFamilyIndex},
                                   commandPool{std::move(odevice.commandPool)},
                                   swapchainKHR{std::move(odevice.swapchainKHR)},
                                   swapchainExtent{std::move(odevice.swapchainExtent)},
                                   depthFormat{odevice.depthFormat},
                                   vkSwapChainBuffers{std::move(odevice.vkSwapChainBuffers)},
                                   depthImage{std::move(odevice.depthImage)},
                                   depthImageView{std::move(odevice.depthImageView)},
                                   depthImageMemory{std::move(odevice.depthImageMemory)},
                                   mvpBuffer{std::move(odevice.mvpBuffer)},
                                   mvpMemory{std::move(odevice.mvpMemory)},
                                   descriptSlBs{std::move(odevice.descriptSlBs)},
                                   descriptorSetLayout{std::move(odevice.descriptorSetLayout)},
                                   pipelineLayout{std::move(odevice.pipelineLayout)},
                                   poolSize{std::move(odevice.poolSize)},
                                   descriptorPoll{std::move(odevice.descriptorPoll)},
                                   descriptorSets{std::move(odevice.descriptorSets)},
                                   renderPass{std::move(odevice.renderPass)},
                                   frameBuffers{std::move(odevice.frameBuffers)},
                                   vkPipelineCache{std::move(odevice.vkPipelineCache)},
                                   graphicsPipeline{std::move(odevice.graphicsPipeline)} {

        }

        Device() = delete;

        ~Device() {
            waitIdle();
            destroy();
        }

        vk::Extent2D getSwapchainExtent() {
            return swapchainExtent;
        }

        std::vector<vk::CommandBuffer>
        defaultPoolAllocBuffer(vk::CommandBufferLevel bufferLevel, uint32_t num);

        vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

        vk::UniqueDeviceMemory allocBindImageMemory(vk::Image image, vk::MemoryPropertyFlags flags);

        vk::UniqueDeviceMemory allocMemoryAndWrite(vk::Buffer &buffer, void *pData, size_t dataSize,
                                                   vk::MemoryPropertyFlags memoryPropertyFlags);

        void buildSwapchainViewBuffers(vk::SurfaceKHR &surfaceKHR);

        void buildMVPBufferAndWrite(glm::mat4 MVP);

        void buildRenderpass(vk::SurfaceKHR &surfaceKHR);

        void buildPipeline(uint32_t dataStepSize);

        void drawCmdBuffer(vk::CommandBuffer &cmdBuffer, vk::Buffer vertexBuffer);
    };

    class Instance : public vk::Instance {
        std::vector<vk::PhysicalDevice> vkPhysicalDevices = enumeratePhysicalDevices();
        std::unique_ptr<tt::Device> upDevice;
        vk::UniqueSurfaceKHR surfaceKHR;

    public:
        Instance(vk::Instance &&ins) : vk::Instance{std::move(ins)} {

        }
        Instance(Instance &&i) : vk::Instance{std::move(i)}, vkPhysicalDevices{std::move(i.vkPhysicalDevices)},
                                 surfaceKHR{std::move(i.surfaceKHR)},upDevice{std::move(i.upDevice)} {

        }

        vk::PhysicalDevice &defaultPhyDevice() {
            return vkPhysicalDevices[0];
        }

        tt::Device &defaultDevice() {
            return *upDevice;
        }

        vk::SurfaceKHR &defaultSurface() {
            return surfaceKHR.get();
        }

        uint32_t queueFamilyPropertiesFindFlags(vk::QueueFlags);

        void connectWSI(ANativeWindow* window) {
            surfaceKHR = createAndroidSurfaceKHRUnique(
                    vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
                                                    window});
        }
        void disconnectWSI() {
            surfaceKHR.reset();
        }

        tt::Device connectToDevice();
        void connectDevice();
        void disconnectDevice(){
            upDevice.reset();
        }
        ~Instance() {
            destroy();
        }

    };

    Instance createInstance();
}
#endif //STBOX_UTIL_H
