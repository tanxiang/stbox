//
// Created by ttand on 18-2-12.
//
#include "vulkan.hpp"

#include "stboxvk.hh"
#include <iostream>
//#include <cassert>
#include "main.hh"

#define GLM_FORCE_RADIANS

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "util.hh"
#include "cube_data.hh"



int st_main_test(int argc, char *argv[]) {
    std::cout << "main_test" << std::endl;

    auto ttInstance = tt::createInstance();

    auto default_queue_index = ttInstance.queueFamilyPropertiesFindFlags(
            vk::QueueFlagBits::eGraphics);
    std::cout << "ttInstance.defaultPhyDevice().createDevice():" << std::endl;

    tt::Device ttDevice {ttInstance.connectToDevice()};

    auto cmdBuf = ttDevice.defaultPoolAllocBuffer(vk::CommandBufferLevel::ePrimary, 1);


    ttDevice.buildSwapchainViewBuffers(ttInstance.defaultSurface());

    auto swapchainExtent = ttDevice.getSwapchainExtent();
    auto Projection = glm::perspective(glm::radians(45.0f),
                                       static_cast<float>(swapchainExtent.width) /
                                       static_cast<float>(swapchainExtent.height), 0.1f,
                                       100.0f);
    auto View = glm::lookAt(glm::vec3(-5, 3, -10),  // Camera is at (-5,3,-10), in World Space
                            glm::vec3(0, 0, 0),     // and looks at the origin
                            glm::vec3(0, -1,
                                      0)     // Head is up (set to 0,-1,0 to look upside-down)
    );
    auto Model = glm::mat4{1.0f};
    // Vulkan clip space has inverted Y and half Z.
    auto Clip = glm::mat4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
                          0.0f, 0.0f, 0.5f, 1.0f};

    ttDevice.buildMVPBufferAndWrite(Clip * Projection * View * Model);

    ttDevice.buildRenderpass(ttInstance.defaultSurface());


    auto vertexBuffer = ttDevice.createBufferUnique(
            vk::BufferCreateInfo{
                    vk::BufferCreateFlags(),
                    sizeof(g_vb_solid_face_colors_Data),
                    vk::BufferUsageFlagBits::eVertexBuffer});
    auto vertexMemory = ttDevice.allocMemoryAndWrite(vertexBuffer.get(),
                                                     (void*)&g_vb_solid_face_colors_Data,
                                                     sizeof(g_vb_solid_face_colors_Data),
                                                     vk::MemoryPropertyFlags() |
                                                     vk::MemoryPropertyFlagBits::eHostVisible |
                                                     vk::MemoryPropertyFlagBits::eHostCoherent);

    ttDevice.buildPipeline(sizeof(g_vb_solid_face_colors_Data[0]));

    ttDevice.drawCmdBuffer(cmdBuf[0],vertexBuffer.get());

    return 0;
}