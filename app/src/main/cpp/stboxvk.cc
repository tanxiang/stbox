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



uint32_t draw_run(tt::Device &ttDevice,vk::SurfaceKHR &surfaceKHR) {
    //std::cout << "draw_run" << std::endl;
    ttDevice.buildPipeline(sizeof(g_vb_solid_face_colors_Data[0]));

    auto & cmdBuf = ttDevice.defaultPoolAllocBuffer();

    auto swapchainExtent = ttDevice.getSwapchainExtent();
    auto Projection = glm::perspective(glm::radians(45.0f),
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

    ttDevice.updateMVPBuffer(Clip * Projection * glm::rotate(View, glm::radians(
            (float) std::chrono::steady_clock::now().time_since_epoch().count() / 10000000.0f),
                                                             glm::vec3(1.0f, 0.0f, 0.0f)) * Model);

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

    return ttDevice.drawCmdBuffer(cmdBuf[0].get(), vertexBuffer.get());

}