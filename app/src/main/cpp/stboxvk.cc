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
#include "onnx.hh"

vk::Extent2D AndroidGetWindowSize(android_app *Android_application) {
    // On Android, retrieve the window size from the native window.
    assert(Android_application != nullptr);
    return vk::Extent2D{ANativeWindow_getWidth(Android_application->window),
                        ANativeWindow_getHeight(Android_application->window)};
}

namespace tt {

    void stboxvk::initData(android_app *app, tt::Instance &instance) {
        Onnx nf{"/storage/0123-4567/nw/mobilenetv2-1.0.onnx"};
    }
    void stboxvk::initDevice(android_app *app,tt::Instance &instance,vk::PhysicalDevice &physicalDevice,int queueIndex,vk::Format rederPassFormat) {
        devicePtr = instance.connectToDevice(physicalDevice,queueIndex);//reconnect
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

        devicePtr->renderPass = devicePtr->createRenderpass(rederPassFormat);

        devicePtr->mvpBuffer = devicePtr->createBufferAndMemory(sizeof(glm::mat4),
                                                                vk::BufferUsageFlagBits::eUniformBuffer,
                                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                                vk::MemoryPropertyFlagBits::eHostCoherent);
        std::vector<VertexUV> vertices{
                {{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-1.0f, 1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
        };


        devicePtr->pipelineLayout = devicePtr->createPipelineLayout(descriptorSetLayout);
        devicePtr->graphicsPipeline = devicePtr->createPipeline(sizeof(decltype(vertices)::value_type), app,
                                                                devicePtr->pipelineLayout.get());
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
        devicePtr->sampleImage =devicePtr->createImageAndMemory(vk::Format::eR8G8B8A8Unorm,vk::Extent3D{static_cast<uint32_t>(tex2d[0].extent().x),static_cast<uint32_t>(tex2d[0].extent().y),1},
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
                    std::get<vk::UniqueImage>(devicePtr->sampleImage).get(),
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
                                                                   std::get<vk::UniqueImage>(devicePtr->sampleImage).get(),
                                                                   vk::ImageLayout::eTransferDstOptimal,
                                                                   bufferCopyRegion);
                                                           vk::ImageMemoryBarrier imageMemoryBarrierToGeneral{
                                                                   vk::AccessFlagBits::eTransferWrite,
                                                                   vk::AccessFlagBits::eShaderRead,
                                                                   vk::ImageLayout::eTransferDstOptimal,
                                                                   vk::ImageLayout::eShaderReadOnlyOptimal, 0, 0,
                                                                   std::get<vk::UniqueImage>(
                                                                           devicePtr->sampleImage).get(),
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

            auto copyFence = devicePtr->submitCmdBuffer(copyCmd[0].get());
            devicePtr->waitFence(copyFence.get());
        }

        devicePtr->sampler = devicePtr->createSampler(tex2d.levels());
        auto descriptorImageInfo = devicePtr->getDescriptorImageInfo(devicePtr->sampleImage,devicePtr->sampler.get());
        auto descriptorBufferInfo = devicePtr->getDescriptorBufferInfo(devicePtr->mvpBuffer);

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
    }

    void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
        assert(instance);
        auto surface = instance.connectToWSI(app->window);
        if (!devicePtr || !devicePtr->checkSurfaceSupport(surface.get())){
            auto phyDevices = instance->enumeratePhysicalDevices();
            auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(phyDevices[0],
                                                                     vk::QueueFlagBits::eGraphics,
                                                                     surface.get());
            auto defaultDeviceFormats = phyDevices[0].getSurfaceFormatsKHR(surface.get());

            initDevice(app,instance,phyDevices[0],graphicsQueueIndex,defaultDeviceFormats[0].format);

        }


        swapchainPtr = std::make_unique<tt::Swapchain>(std::move(surface), *devicePtr, AndroidGetWindowSize(app));

        auto swapchainExtent = swapchainPtr->getSwapchainExtent();

        auto Projection =glm::perspective(glm::radians(60.0f), static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 256.0f);

        auto View = glm::lookAt(
                glm::vec3(8, 3, 10),  // Camera is at (-5,3,-10), in World Space
                glm::vec3(0, 0, 0),     // and looks at the origin
                glm::vec3(0, 1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
        );

        auto mvpMat4 =  Projection  * View ;

        {
            auto mvpBuffer_ptr = devicePtr->mapMemoryAndSize(devicePtr->mvpBuffer);
            //todo copy to buffer
            memcpy(mvpBuffer_ptr.get(), &mvpMat4, std::get<size_t>(devicePtr->mvpBuffer));
        }

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
            cmdHandleRenderpassBegin.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, devicePtr->pipelineLayout.get(), 0,
                                                        tmpDescriptorSets, std::vector<uint32_t>{});
            vk::DeviceSize offsets[1] = {0};
            cmdHandleRenderpassBegin.bindVertexBuffers(0, 1, &std::get<vk::UniqueBuffer>(devicePtr->vertexBuffer).get(), offsets);
            cmdHandleRenderpassBegin.bindIndexBuffer(std::get<vk::UniqueBuffer>(devicePtr->indexBuffer).get(),0,vk::IndexType::eUint32);
            cmdHandleRenderpassBegin.drawIndexed(6,1,0,0,0);
        });
        devicePtr->submitCmdBufferAndWait(*swapchainPtr, devicePtr->mianBuffers);
    }

    void stboxvk::cleanWindow() {
        std::cout << __func__ << std::endl;
        swapchainPtr.reset();
        //devicePtr.reset();
    }

}
