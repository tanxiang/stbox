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
        {
            auto mvpBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->mvpBuffer);
            //todo copy to buffer
            memcpy(mvpBuffer_ptr.get(), &View, std::get<size_t>(devicePtr->mvpBuffer));
        }
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
        {
            auto vertexBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->vertexBuffer);
            memcpy(vertexBuffer_ptr.get(),vertices.data(),std::get<size_t>(devicePtr->vertexBuffer));
        }
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        devicePtr->indexBuffer = devicePtr->createBufferAndMemory(
                sizeof(decltype(indices)::value_type) * indices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
        {
            auto indexBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->indexBuffer);
            memcpy(indexBuffer_ptr.get(), indices.data(), std::get<size_t>(devicePtr->indexBuffer));
        }
        devicePtr->mianBuffers = devicePtr->createCmdBuffers(std::get<vk::UniqueBuffer>(devicePtr->vertexBuffer).get(),
                                              *swapchainPtr,
                                              pipelineLayout.get());

        gli::texture2d tex2d;
        {
            auto fileContent = loadDataFromAssets("textures/vulkan_11_rgba.ktx", app);
            tex2d = gli::texture2d{gli::load(fileContent.data(), fileContent.size())};
        }
        auto sampleImage =devicePtr->createImageAndMemory(vk::Format::eR8G8B8A8Unorm,vk::Extent3D{static_cast<uint32_t>(tex2d[0].extent().x),static_cast<uint32_t>(tex2d[0].extent().y),1},
                                                          vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferDst|vk::ImageUsageFlagBits::eStorage,
                                                          tex2d.levels(),
                                                          vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
                                                          vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        auto copyCmd = (*devicePtr)->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{devicePtr->getCommandPool(),vk::CommandBufferLevel::ePrimary,1});;

        //std::cout<<"return initWindow release"<<std::endl;
        //auto copyCmd = (*devicePtr)->allocateCommandBuffersUnique();
         {
             commandBufferBeginHandle copyCmdHandle{copyCmd[0]};
             vk::ImageSubresourceRange imageSubresourceRange{
                 vk::ImageAspectFlagBits::eColor,
                 0,tex2d.levels(),0,1
             };
             vk::ImageMemoryBarrier imageMemoryBarrierToDest{
                 vk::AccessFlags{},vk::AccessFlagBits::eTransferWrite,
                 vk::ImageLayout::eUndefined,vk::ImageLayout::eTransferDstOptimal,0,0,std::get<vk::UniqueImage>(sampleImage).get(),
                 imageSubresourceRange
             };
             copyCmdHandle.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,
                                           vk::DependencyFlags{},
                                           0,nullptr,
                                           0,nullptr,
                                           1,&imageMemoryBarrierToDest);
             std::vector<vk::BufferImageCopy> bufferCopyRegion;
             for(int i = 0 ,offset = 0 ;i < tex2d.levels(); ++i){
                 bufferCopyRegion.emplace_back(offset,0,0,
                                               vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor,i,0,1},
                                               vk::Offset3D{},
                                               vk::Extent3D{tex2d[i].extent().x,tex2d[i].extent().y,1});
                 offset += tex2d[i].size();
             }
             auto sampleBuffer = devicePtr->createBufferAndMemory(
                     tex2d.size(),
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent);
             {
                 auto sampleBufferPtr = devicePtr->mapMemoryAndSize(sampleBuffer);
                 memcpy(sampleBufferPtr.get(), tex2d.data(), tex2d.size());
             }
             copyCmdHandle.copyBufferToImage(std::get<vk::UniqueBuffer>(sampleBuffer).get(),std::get<vk::UniqueImage>(sampleImage).get(),vk::ImageLayout::eTransferDstOptimal,bufferCopyRegion);
             vk::ImageMemoryBarrier imageMemoryBarrierToGeneral{
                     vk::AccessFlagBits::eTransferWrite,vk::AccessFlags{},
                     vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::eGeneral,0,0,std::get<vk::UniqueImage>(sampleImage).get(),
                     imageSubresourceRange
             };
             copyCmdHandle.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,vk::PipelineStageFlagBits::eAllCommands,
                                           vk::DependencyFlags{},
                                           0,nullptr,
                                           0,nullptr,
                                           1,&imageMemoryBarrierToGeneral);
         }

        auto fence = (*devicePtr)->createFenceUnique(vk::FenceCreateInfo{});
        std::array<vk::SubmitInfo, 1> submitInfos{
                vk::SubmitInfo{
                        0, nullptr, nullptr,
                        1, &copyCmd[0].get()
                }
        };
        devicePtr->graphsQueue().submit(submitInfos, vk::Fence{});
        (*devicePtr)->waitForFences(1,&fence.get(),1,100000000000);

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