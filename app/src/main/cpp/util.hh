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
#include <thread>
#include <queue>
#include <condition_variable>
#include <android_native_app_glue.h>
#include <iostream>

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

#define SWAPCHAIN_NUM 3
namespace tt {
    class Device : public vk::Device {
        vk::PhysicalDevice &physicalDevice;
        uint32_t queueFamilyIndex;
        vk::UniqueCommandPool commandPool;
        std::vector<vk::UniqueCommandBuffer> commandBuffers;
        vk::UniqueSwapchainKHR swapchainKHR;
        vk::Extent2D swapchainExtent;
        vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
        std::vector<std::tuple<vk::Image, vk::UniqueImageView, vk::UniqueFramebuffer, vk::UniqueFence>> vkSwapChainBuffers;
        vk::UniqueImage depthImage;
        vk::UniqueImageView depthImageView;
        vk::UniqueDeviceMemory depthImageMemory;
        std::array<vk::UniqueBuffer, 2> mvpBuffer{createBufferUnique(
                vk::BufferCreateInfo{
                        vk::BufferCreateFlags(),
                        sizeof(glm::mat4),
                        vk::BufferUsageFlagBits::eUniformBuffer}), createBufferUnique(
                vk::BufferCreateInfo{
                        vk::BufferCreateFlags(),
                        sizeof(glm::mat4),
                        vk::BufferUsageFlagBits::eUniformBuffer})};
        std::array<vk::MemoryRequirements, 2> mvpBufferMemoryRqs{
                getBufferMemoryRequirements(mvpBuffer[0].get()),
                getBufferMemoryRequirements(mvpBuffer[1].get())
        };
        vk::UniqueDeviceMemory mvpMemory{allocateMemoryUnique(vk::MemoryAllocateInfo{
                mvpBufferMemoryRqs[0].size + mvpBufferMemoryRqs[1].size,
                findMemoryTypeIndex(mvpBufferMemoryRqs[0].memoryTypeBits,
                                    vk::MemoryPropertyFlags() |
                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent)
        })};

        vk::UniqueDescriptorSetLayout ttcreateDescriptorSetLayoutUnique() {
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
            return createDescriptorSetLayoutUnique(
                    vk::DescriptorSetLayoutCreateInfo{
                            vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(),
                            descriptSlBs.data()
                    });
        }

        vk::UniqueDescriptorSetLayout descriptorSetLayout{ttcreateDescriptorSetLayoutUnique()};

        vk::UniquePipelineLayout pipelineLayout = createPipelineLayoutUnique(
                vk::PipelineLayoutCreateInfo{
                        vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0, nullptr
                });

        vk::UniqueDescriptorPool ttcreateDescriptorPoolUnique() {
            std::array<vk::DescriptorPoolSize, 2> poolSize{
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
                    vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2}};
            return createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                    vk::DescriptorPoolCreateFlags(), 2, poolSize.size(), poolSize.data()});
        }

        vk::UniqueDescriptorPool descriptorPoll = ttcreateDescriptorPoolUnique();

        std::vector<vk::UniqueDescriptorSet> ttcreateDescriptorSets() {
            std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts{descriptorSetLayout.get(),
                                                                        descriptorSetLayout.get()};
            return allocateDescriptorSetsUnique(
                    vk::DescriptorSetAllocateInfo{
                            descriptorPoll.get(), descriptorSetLayouts.size(),
                            descriptorSetLayouts.data()});
        }

        std::vector<vk::UniqueDescriptorSet> descriptorSets{ttcreateDescriptorSets()};
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineCache vkPipelineCache = createPipelineCacheUnique(
                vk::PipelineCacheCreateInfo{});
        vk::UniquePipeline graphicsPipeline;

        std::unique_ptr<std::thread> submitThread;
        std::queue<uint32_t> frameSubmitIndex;
        bool submitExitFlag = false;
        std::mutex mutexDraw;
        std::condition_variable cvDraw;

        uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);

        vk::UniqueShaderModule loadShaderFromFile(const char *filePath,
                                                  android_app *androidAppCtx);

    public:
        Device(vk::Device dev, vk::PhysicalDevice &phy, uint32_t qidx) :
                vk::Device{dev}, physicalDevice{phy}, queueFamilyIndex{qidx},
                commandPool{createCommandPoolUnique(
                        vk::CommandPoolCreateInfo{
                                vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qidx})
                }, commandBuffers{
                allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{commandPool.get(),
                                                                           vk::CommandBufferLevel::ePrimary,
                                                                           1})} {
            auto mvpBufferInfo0 = vk::DescriptorBufferInfo{mvpBuffer[0].get(), 0,
                                                           sizeof(glm::mat4)};
            bindBufferMemory(mvpBuffer[0].get(), mvpMemory.get(), 0);
            auto mvpBufferInfo1 = vk::DescriptorBufferInfo{mvpBuffer[1].get(),
                                                           mvpBufferMemoryRqs[0].size,
                                                           sizeof(glm::mat4)};
            bindBufferMemory(mvpBuffer[1].get(), mvpMemory.get(), mvpBufferMemoryRqs[0].size);

            assert(descriptorSets.size() == 2);
            std::cout << "descriptorSets.size()" << descriptorSets.size() << std::endl;
            updateDescriptorSets(
                    std::vector<vk::WriteDescriptorSet>{
                            vk::WriteDescriptorSet{descriptorSets[0].get(), 0, 0, 1,
                                                   vk::DescriptorType::eUniformBuffer,
                                                   nullptr, &mvpBufferInfo0},
                            vk::WriteDescriptorSet{descriptorSets[1].get(), 0, 0, 1,
                                                   vk::DescriptorType::eUniformBuffer,
                                                   nullptr, &mvpBufferInfo1}},

                    nullptr);    //todo use_texture
        }

        Device(Device &&odevice) : physicalDevice{odevice.physicalDevice},
                                   queueFamilyIndex{odevice.queueFamilyIndex},
                                   commandPool{std::move(odevice.commandPool)},
                                   commandBuffers{std::move(odevice.commandBuffers)},
                                   swapchainKHR{std::move(odevice.swapchainKHR)},
                                   swapchainExtent{std::move(odevice.swapchainExtent)},
                                   depthFormat{odevice.depthFormat},
                                   vkSwapChainBuffers{std::move(odevice.vkSwapChainBuffers)},
                                   depthImage{std::move(odevice.depthImage)},
                                   depthImageView{std::move(odevice.depthImageView)},
                                   depthImageMemory{std::move(odevice.depthImageMemory)},
                                   mvpBuffer{std::move(odevice.mvpBuffer)},
                                   mvpMemory{std::move(odevice.mvpMemory)},
                                   descriptorSetLayout{std::move(odevice.descriptorSetLayout)},
                                   pipelineLayout{std::move(odevice.pipelineLayout)},
                                   descriptorPoll{std::move(odevice.descriptorPoll)},
                                   descriptorSets{std::move(odevice.descriptorSets)},
                                   renderPass{std::move(odevice.renderPass)},
                                   vkPipelineCache{std::move(odevice.vkPipelineCache)},
                                   graphicsPipeline{std::move(odevice.graphicsPipeline)} {}

        Device() = delete;

        ~Device() {
            waitIdle();
            destroy();
        }

        vk::Extent2D getSwapchainExtent() {
            return swapchainExtent;
        }

        std::vector<vk::UniqueCommandBuffer> &defaultPoolAllocBuffer() {
            return commandBuffers;
        }

        vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

        vk::UniqueDeviceMemory allocBindImageMemory(vk::Image image, vk::MemoryPropertyFlags flags);

        vk::UniqueDeviceMemory allocMemoryAndWrite(vk::Buffer &buffer, void *pData, size_t dataSize,
                                                   vk::MemoryPropertyFlags memoryPropertyFlags);

        void buildSwapchainViewBuffers(vk::SurfaceKHR &surfaceKHR);

        //void buildMVPBufferAndWrite(glm::mat4 MVP);

        void updateMVPBuffer(glm::mat4 MVP);

        void buildRenderpass(vk::SurfaceKHR &surfaceKHR);

        void buildPipeline(uint32_t dataStepSize, android_app *app);

        //void renderPassReset() {
        //    vkSwapChainBuffers.clear();
        //    renderPass.reset();
        //}

        uint32_t
        drawCmdBuffer(vk::CommandBuffer &cmdBuffer, glm::mat4 MVP, vk::Buffer vertexBuffer);

        void buildSubmitThread(vk::SurfaceKHR &surfaceKHR);

        void stopSubmitThread();

        void resetDraw() {
            while (!frameSubmitIndex.empty()) {
                swapchainPresent();
            }
        }

        void swapchainPresent();

    };

    class Instance : public vk::Instance {
        std::vector<vk::PhysicalDevice> vkPhysicalDevices = enumeratePhysicalDevices();
        std::unique_ptr<tt::Device> upDevice;
        vk::UniqueSurfaceKHR surfaceKHR;
        bool focus = false;
    public:
        Instance(vk::Instance &&ins) : vk::Instance{std::move(ins)} {

        }

        Instance(Instance &&i) : vk::Instance{std::move(i)},
                                 vkPhysicalDevices{std::move(i.vkPhysicalDevices)},
                                 surfaceKHR{std::move(i.surfaceKHR)},
                                 upDevice{std::move(i.upDevice)} {

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

        void connectWSI(ANativeWindow *window) {
            surfaceKHR = createAndroidSurfaceKHRUnique(
                    vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
                                                    window});
        }

        void disconnectWSI() {
            upDevice->resetDraw();
            surfaceKHR.reset();
        }

        tt::Device connectToDevice();

        bool connectedDevice() {
            return upDevice.operator bool();
        }

        void connectDevice();

        void disconnectDevice() {
            upDevice.reset();
        }

        void setFocus() {
            focus = true;
        }

        void unsetFocus() {
            focus = false;
        }

        bool isFocus() {
            return focus;
        }

        ~Instance() {
            destroy();
        }

    };

    Instance createInstance();

}
#endif //STBOX_UTIL_H
