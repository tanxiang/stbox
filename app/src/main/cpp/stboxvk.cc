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
        devicePtr->buildDescriptorSets(descriptorSetLayout);
        auto pipelineLayout = devicePtr->createPipelineLayout(descriptorSetLayout);
        devicePtr->buildPipeline(sizeof(Vertex), app, *swapchainPtr, pipelineLayout.get());

        auto textuerContent = tt::loadDataFromAssets("textures/vulkan_11_rgba.ktx", app);
        gli::texture2d tex2D{gli::load(textuerContent.data(), textuerContent.size())};
        //auto texture = device.createImageAndMemory();
        assert(!tex2D.empty());


        static auto View = glm::lookAt(
                glm::vec3(-5, 3, -10),  // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),     // and looks at the origin
                glm::vec3(0, -1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
        );
        glm::rotate(View, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //std::cout<<__func__<< static_cast<VkBuffer>(std::get<vk::UniqueBuffer>(mvpBuffer).get())<<std::endl;
        devicePtr->mvpBuffer = devicePtr->createBufferAndMemory(sizeof(glm::mat4),
                                                 vk::BufferUsageFlagBits::eUniformBuffer,
                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                 vk::MemoryPropertyFlagBits::eHostCoherent);
        //std::cout<<__func__<< static_cast<VkBuffer>(std::get<vk::UniqueBuffer>(mvpBuffer).get())<<std::endl;


        auto mvpBuffer_ptr = devicePtr->mapBufferAndMemory(devicePtr->mvpBuffer);
        //todo copy to buffer
        memcpy(mvpBuffer_ptr.get(), &View, std::get<size_t>(devicePtr->mvpBuffer));

        std::vector<VertexUV> vertices{
                {{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-1.0f, 1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
        };

        devicePtr->vertexBuffer = devicePtr->createBufferAndMemory(
                sizeof(decltype(vertices)::value_type) * vertices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto vertexBuffer_ptr = devicePtr->mapBufferAndMemory(devicePtr->vertexBuffer);

        memcpy(vertexBuffer_ptr.get(), vertices.data(), std::get<size_t>(devicePtr->vertexBuffer));

        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        devicePtr->indexBuffer = devicePtr->createBufferAndMemory(
                sizeof(decltype(indices)::value_type) * indices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        auto indexBuffer_ptr = devicePtr->mapBufferAndMemory(devicePtr->indexBuffer);

        memcpy(indexBuffer_ptr.get(), indices.data(), std::get<size_t>(devicePtr->indexBuffer));

        devicePtr->mianBuffers = devicePtr->createCmdBuffers(std::get<vk::UniqueBuffer>(devicePtr->vertexBuffer).get(),
                                              *swapchainPtr,
                                              pipelineLayout.get());
        //mianBuffers.clear();
        //std::cout<<"return initWindow release"<<std::endl;
    }

    void stboxvk::cleanWindow() {
        std::cout << __func__ << std::endl;
        swapchainPtr.reset();
        devicePtr.reset();
    }

}
#if 0
uint32_t draw_run(tt::Device &ttDevice, vk::SurfaceKHR &surfaceKHR) {
    //std::cout << "draw_run" << std::endl;

    //auto &cmdBuf = ttDevice.defaultPoolAllocBuffer();

    auto swapchainExtent = ttDevice.getSwapchainExtent();
    static auto Projection = glm::perspective(glm::radians(45.0f),
                                       static_cast<float>(swapchainExtent.width) /
                                       static_cast<float>(swapchainExtent.height), 0.1f,
                                       100.0f);
    static auto View = glm::lookAt(
            glm::vec3(-5, 3, -10),  // Camera is at (-5,3,-10), in World Space
            glm::vec3(0, 0, 0),     // and looks at the origin
            glm::vec3(0, -1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
    );
    glm::rotate(View, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    static auto Model = glm::mat4{1.0f};
    // Vulkan clip space has inverted Y and half Z.
    static auto Clip = glm::mat4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
                                 0.0f,
                                 0.0f, 0.0f, 0.5f, 1.0f};

    static vk::UniqueBuffer vertexBuffer;
    static vk::UniqueDeviceMemory vertexMemory;
    if (!vertexBuffer || !vertexMemory) {
        vertexBuffer = ttDevice.createBufferUnique(
                vk::BufferCreateInfo{
                        vk::BufferCreateFlags(),
                        sizeof(g_vb_solid_face_colors_Data),
                        vk::BufferUsageFlagBits::eVertexBuffer});
        vertexMemory = ttDevice.allocMemoryAndWrite(vertexBuffer.get(),
                                                    (void *) &g_vb_solid_face_colors_Data,
                                                    sizeof(g_vb_solid_face_colors_Data),
                                                    vk::MemoryPropertyFlags() |
                                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    return ttDevice.drawCmdBuffer(Clip * Projection * glm::rotate(View, glm::radians(
                                          (float) std::chrono::steady_clock::now().time_since_epoch().count() /
                                          10000000.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * Model,
                                  vertexBuffer.get());

}
#endif