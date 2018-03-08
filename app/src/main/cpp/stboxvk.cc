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

static const char *vertShaderText =
        "#version 400\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (std140, binding = 0) uniform bufferVals {\n"
                "    mat4 mvp;\n"
                "} myBufferVals;\n"
                "layout (location = 0) in vec4 pos;\n"
                "layout (location = 1) in vec4 inColor;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main() {\n"
                "   outColor = inColor;\n"
                "   gl_Position = myBufferVals.mvp * pos;\n"
                "}\n";

static const char *fragShaderText =
        "#version 400\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (location = 0) in vec4 color;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main() {\n"
                "   outColor = color;\n"
                "}\n";

int st_main_test(int argc, char *argv[]) {
    std::cout << "main_test" << std::endl;

    auto ttInstance = tt::createInstance();

    auto default_queue_index = ttInstance.queueFamilyPropertiesFindFlags(
            vk::QueueFlagBits::eGraphics);
    std::cout << "ttInstance.defaultPhyDevice().createDevice():" << std::endl;

    auto ttDevice = ttInstance.connectToDevice();

    auto cmd_buf = ttDevice.defaultPoolAllocBuffer(vk::CommandBufferLevel::ePrimary, 1);


    ttDevice.buildSwapchainViewBuffers(ttInstance.defaultSurface());

    auto swapchainExtent = ttDevice.getSwapchainExtent();
    auto Projection = glm::perspective(glm::radians(45.0f), static_cast<float>(swapchainExtent.width) /
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


    auto vertex_buffer = ttDevice.createBuffer(
            vk::BufferCreateInfo{
                    vk::BufferCreateFlags(),
                    sizeof(g_vb_solid_face_colors_Data),
                    vk::BufferUsageFlagBits::eVertexBuffer});
    auto vertex_buffer_memory_rq = ttDevice.getBufferMemoryRequirements(vertex_buffer);

    typeIndex = UINT32_MAX;
    typeBits = vertex_buffer_memory_rq.memoryTypeBits;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memory_properties.memoryTypes[i].propertyFlags &
                 (vk::MemoryPropertyFlags() | vk::MemoryPropertyFlagBits::eHostVisible |
                  vk::MemoryPropertyFlagBits::eHostCoherent))
                == (vk::MemoryPropertyFlags() | vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent)) {
                typeIndex = i;
                break;
            }
        }
        typeBits >>= 1;
    }

    if (typeIndex == UINT32_MAX) {
        std::cout << "memory_properties no match exit!" << std::endl;
        exit(-1);
    }
    std::cout << "vertex_memory:alloc index:" << typeIndex << std::endl;
    auto vertex_memory = ttDevice.allocateMemory(vk::MemoryAllocateInfo{
            vertex_buffer_memory_rq.size, typeIndex
    });
    pData = ttDevice.mapMemory(vertex_memory, 0, vertex_buffer_memory_rq.size,
                               vk::MemoryMapFlagBits());
    memcpy(pData, &g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));
    ttDevice.unmapMemory(vertex_memory);
    pData = nullptr;
    ttDevice.bindBufferMemory(vertex_buffer, vertex_memory, 0);

    auto vert_shader_spirv = GLSLtoSPV(vk::ShaderStageFlagBits::eVertex, vertShaderText);
    std::cout << "vert_shader_spirv len:" << vert_shader_spirv.size() << 'x'
              << sizeof(decltype(vert_shader_spirv)
    ::value_type)<<std::endl;
    auto farg_shader_spirv = GLSLtoSPV(vk::ShaderStageFlagBits::eFragment, fragShaderText);
    std::cout << "farg_shader_spirv len:" << farg_shader_spirv.size() << 'x'
              << sizeof(decltype(farg_shader_spirv)
    ::value_type)<<std::endl;
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos{
            vk::PipelineShaderStageCreateInfo{
                    vk::PipelineShaderStageCreateFlags(),
                    vk::ShaderStageFlagBits::eVertex,
                    ttDevice.createShaderModule(vk::ShaderModuleCreateInfo{
                            vk::ShaderModuleCreateFlags(), vert_shader_spirv.size() *
                                                           sizeof(decltype(vert_shader_spirv)::value_type), vert_shader_spirv.data()
                    }), "main"
            },
            vk::PipelineShaderStageCreateInfo{
                    vk::PipelineShaderStageCreateFlags(),
                    vk::ShaderStageFlagBits::eFragment,
                    ttDevice.createShaderModule(vk::ShaderModuleCreateInfo{
                            vk::ShaderModuleCreateFlags(), farg_shader_spirv.size() *
                                                           sizeof(decltype(farg_shader_spirv)::value_type), farg_shader_spirv.data()
                    }), "main"
            }
    };



    auto vk_pipeline_cache = ttDevice.createPipelineCache(vk::PipelineCacheCreateInfo{});

    std::array<vk::VertexInputBindingDescription, 1> vertexInputBindingDescriptions{
            vk::VertexInputBindingDescription{
                    0, sizeof(g_vb_solid_face_colors_Data[0]),
                    vk::VertexInputRate::eVertex
            }
    };
    std::array<vk::VertexInputAttributeDescription, 2> vertexInputAttributeDescriptions{
            vk::VertexInputAttributeDescription{
                    0, 0, vk::Format::eR32G32B32A32Sfloat, 0
            },
            vk::VertexInputAttributeDescription{
                    1, 0, vk::Format::eR32G32B32A32Sfloat, 16
            }
    };
    vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
            vk::PipelineVertexInputStateCreateFlags(),
            vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
            vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

    };

    vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
            vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList
    };
    //vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};
    vk::Viewport viewport{
            0, 0, swapchainExtent.width, swapchainExtent.height, 0.0f, 1.0f
    };
    vk::Rect2D scissors{vk::Offset2D{}, swapchainExtent};
    vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
            vk::PipelineViewportStateCreateFlags(),
            1, &viewport, 1, &scissors
    };
    vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
            vk::PipelineRasterizationStateCreateFlags(),
            0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, 0,
            0, 0, 0, 1.0f
    };
    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
    vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
            vk::PipelineDepthStencilStateCreateFlags(), true, true, vk::CompareOp::eLessOrEqual,
            false, false,
            vk::StencilOpState{vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                               vk::CompareOp::eAlways},
            vk::StencilOpState{vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                               vk::CompareOp::eAlways},
            0, 0
    };
    vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
    pipelineColorBlendAttachmentState.setColorWriteMask(
            vk::ColorComponentFlags{} | vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
            vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, 1,
            &pipelineColorBlendAttachmentState, {1.0f, 1.0f, 1.0f, 1.0f}
    };
    vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
            vk::PipelineCreateFlags(),
            shaderStageCreateInfos.size(), shaderStageCreateInfos.data(),
            &pipelineVertexInputStateCreateInfo,
            &pipelineInputAssemblyStateCreateInfo,
            nullptr,
            &pipelineViewportStateCreateInfo,
            &pipelineRasterizationStateCreateInfo,
            &pipelineMultisampleStateCreateInfo,
            &pipelineDepthStencilStateCreateInfo,
            &pipelineColorBlendStateCreateInfo,
            &pipelineDynamicStateCreateInfo,
            pipeline_layout,
            render_pass
    };
    auto graphicsPipeline = ttDevice.createGraphicsPipeline(vk_pipeline_cache, pipelineCreateInfo);
    auto imageAcquiredSemaphore = ttDevice.createSemaphore(vk::SemaphoreCreateInfo{});
    auto currentBufferIndex = ttDevice.acquireNextImageKHR(vk_swap_chain, UINT64_MAX,
                                                           imageAcquiredSemaphore, vk::Fence{});
    std::cout << "acquireNextImageKHR:" << vk::to_string(currentBufferIndex.result)
              << currentBufferIndex.value << std::endl;
    std::array<vk::ClearValue, 2> clearValues{
            vk::ClearColorValue{std::array<float, 4>{0.5f, 0.2f, 0.2f, 0.2f}},
            vk::ClearDepthStencilValue{1.0f, 0},
    };
    cmd_buf[0].begin(vk::CommandBufferBeginInfo{});

    cmd_buf[0].beginRenderPass(vk::RenderPassBeginInfo{
            render_pass,
            vk_frame_buffers[currentBufferIndex.value],
            vk::Rect2D{vk::Offset2D{}, swapchainExtent},
            clearValues.size(), clearValues.data()
    }, vk::SubpassContents::eInline);
    cmd_buf[0].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    cmd_buf[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0,
                                  descriptor_sets, std::vector<uint32_t>{});
    vk::DeviceSize offsets[1] = {0};
    cmd_buf[0].bindVertexBuffers(0, 1, &vertex_buffer, offsets);
    cmd_buf[0].draw(12 * 3, 1, 0, 0);
    cmd_buf[0].endRenderPass();
    cmd_buf[0].end();
    auto drawFence = ttDevice.createFence(vk::FenceCreateInfo{});
    vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    std::array<vk::SubmitInfo, 1> submitInfos{
            vk::SubmitInfo{
                    1, &imageAcquiredSemaphore, &pipelineStageFlags,
                    1, cmd_buf.data()
            }
    };
    auto vk_queue = ttDevice.getQueue(default_queue_index, 0);

    vk_queue.submit(submitInfos, drawFence);
    vk::Result waitRet;
    do {
        waitRet = ttDevice.waitForFences(1, &drawFence, true, 1000000);
        std::cout << "waitForFences ret:" << vk::to_string(waitRet) << std::endl;
    } while (waitRet == vk::Result::eTimeout);

    auto presentRet = vk_queue.presentKHR(vk::PresentInfoKHR{
            0, nullptr, 1, &vk_swap_chain, &currentBufferIndex.value
    });
    std::cout << "presentKHR:index" << currentBufferIndex.value << "ret:"
              << vk::to_string(presentRet) << std::endl;
    sleep(1);
    return 0;
}