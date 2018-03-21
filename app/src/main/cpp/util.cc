//
// Created by ttand on 18-2-11.
//
#include <assert.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "util.hh"

// Header files.
#include <android_native_app_glue.h>
#include <cstring>
#include "shaderc/shaderc.hpp"
#include "util.hh"
#include "stboxvk.hh"
#include <map>

using namespace std::chrono_literals;

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

static const std::map<vk::ShaderStageFlagBits, shaderc_shader_kind> shader_map_table{
        {vk::ShaderStageFlagBits::eVertex,                 shaderc_glsl_vertex_shader},
        {vk::ShaderStageFlagBits::eTessellationControl,    shaderc_glsl_tess_control_shader},
        {vk::ShaderStageFlagBits::eTessellationEvaluation, shaderc_glsl_tess_evaluation_shader},
        {vk::ShaderStageFlagBits::eGeometry,               shaderc_glsl_geometry_shader},
        {vk::ShaderStageFlagBits::eFragment,               shaderc_glsl_fragment_shader},
        {vk::ShaderStageFlagBits::eCompute,                shaderc_glsl_compute_shader},
};

//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader) {
    // On Android, use shaderc instead.
    shaderc::Compiler compiler;
    shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(pshader, strlen(pshader), shader_map_table.at(shader_type),
                                      "shader");
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error_msg{"shaderc Error: Id=%d, Msg=%s"};
        error_msg += module.GetCompilationStatus();
        error_msg += module.GetErrorMessage().c_str();
        throw std::runtime_error{error_msg};
    }
    return std::vector<uint32_t>{module.cbegin(), module.cend()};
}

namespace tt {
    Instance createInstance() {
        auto instance_layer_props = vk::enumerateInstanceLayerProperties();
        std::cout << "enumerateInstanceLayerProperties:" << instance_layer_props.size()
                  << std::endl;
        for (auto &prop:instance_layer_props)
            std::cout << prop.layerName << std::endl;
        vk::ApplicationInfo vkAppInfo{"stbox", VK_MAKE_VERSION(1, 0, 0), "stbox",
                                      VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0};
        std::array<const char *, 2> instanceEtensionNames{VK_KHR_SURFACE_EXTENSION_NAME,
                                                          VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
        vk::InstanceCreateInfo vkInstanceInfo{vk::InstanceCreateFlags(), &vkAppInfo,
                                              0, nullptr,
                                              instanceEtensionNames.size(),
                                              instanceEtensionNames.data()};
        return tt::Instance{vk::createInstance(vkInstanceInfo)};
    }

    uint32_t Instance::queueFamilyPropertiesFindFlags(vk::QueueFlags flags) {
        auto queueFamilyProperties = defaultPhyDevice().getQueueFamilyProperties();
        std::cout << "ttInstance.defaultPhyDevice() getQueueFamilyProperties : "
                  << queueFamilyProperties.size() << std::endl;
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
            std::cout << "QueueFamilyProperties : " << i << "\tflags:"
                      << vk::to_string(queueFamilyProperties[i].queueFlags) << std::endl;
            if (defaultPhyDevice().getSurfaceSupportKHR(i, defaultSurface()) &&
                (queueFamilyProperties[i].queueFlags & flags)) {
                std::cout << "default_queue_index :" << i << "\tgetSurfaceSupportKHR:true"
                          << std::endl;
                return i;
            }
        }
        throw std::logic_error{"queueFamilyPropertiesFindFlags Error"};
    }


    void Instance::connectDevice() {
        auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(vk::QueueFlagBits::eGraphics);
        std::array<float, 1> queue_priorities{0.0};
        std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos{
                vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(),
                                          graphicsQueueIndex,
                                          queue_priorities.size(), queue_priorities.data()
                }};
        std::array<const char *, 1> device_extension_names{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        upDevice.reset(new Device{defaultPhyDevice().createDevice(
                vk::DeviceCreateInfo {vk::DeviceCreateFlags(),
                                      device_queue_create_infos.size(),
                                      device_queue_create_infos.data(),
                                      0, nullptr, device_extension_names.size(),
                                      device_extension_names.data()}),
                                  defaultPhyDevice(),
                                  graphicsQueueIndex});
    }

    vk::SurfaceFormatKHR Device::getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR) {
        auto vkDefaultDeviceFormats = physicalDevice.getSurfaceFormatsKHR(surfaceKHR);
        std::cout << "vk_default_device_formats.size():" << vkDefaultDeviceFormats.size() << " :"
                  << std::endl;
        for (auto &format:vkDefaultDeviceFormats) {
            std::cout << "\t\t" << vk::to_string(format.format) << std::endl;
        }
        return vkDefaultDeviceFormats[0];
    }


    uint32_t Device::findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags) {
        auto memoryProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((memoryTypeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                    return i;
                }
            }
            memoryTypeBits >>= 1;
        }
        std::cout << "memoryProperties no match exit!" << std::endl;
        exit(-1);//todo throw
    }

    vk::UniqueDeviceMemory
    Device::allocBindImageMemory(vk::Image image, vk::MemoryPropertyFlags flags) {
        auto imageMemoryRq = getImageMemoryRequirements(image);
        auto typeIndex = findMemoryTypeIndex(imageMemoryRq.memoryTypeBits, flags);
        auto imageMemory = allocateMemoryUnique(vk::MemoryAllocateInfo{
                imageMemoryRq.size, findMemoryTypeIndex(imageMemoryRq.memoryTypeBits, flags)
        });
        std::cout << "ImageMemory:alloc index:" << typeIndex << std::endl;
        bindImageMemory(image, imageMemory.get(), 0);
        return imageMemory;
    }

    vk::UniqueDeviceMemory
    Device::allocMemoryAndWrite(vk::Buffer &buffer, void *pData, size_t dataSize,
                                vk::MemoryPropertyFlags memoryPropertyFlags) {
        auto memoryRequirements = getBufferMemoryRequirements(buffer);
        auto typeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                             memoryPropertyFlags);
        std::cout << "vertex_memory:alloc index:" << typeIndex << std::endl;
        auto memoryUnique = allocateMemoryUnique(vk::MemoryAllocateInfo{
                memoryRequirements.size, typeIndex
        });
        auto pMemory = mapMemory(memoryUnique.get(), 0, memoryRequirements.size,
                                 vk::MemoryMapFlagBits());
        memcpy(pMemory, pData, dataSize);
        unmapMemory(memoryUnique.get());
        bindBufferMemory(buffer, memoryUnique.get(), 0);
        return memoryUnique;
    }

    void Device::buildSwapchainViewBuffers(vk::SurfaceKHR &surfaceKHR) {
        auto surfaceCapabilitiesKHR = physicalDevice.getSurfaceCapabilitiesKHR(surfaceKHR);
        //auto surface_present_mods = physicalDevice.getSurfacePresentModesKHR(surfaceKHR);
        std::tie(swapchainExtent.width, swapchainExtent.height) = AndroidGetWindowSize();
        std::cout << "AndroidGetWindowSize() : " << swapchainExtent.width << " x "
                  << swapchainExtent.height << std::endl;
        if (surfaceCapabilitiesKHR.currentExtent.width == 0xFFFFFFFF) {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            if (swapchainExtent.width < surfaceCapabilitiesKHR.minImageExtent.width) {
                swapchainExtent.width = surfaceCapabilitiesKHR.minImageExtent.width;
            } else if (swapchainExtent.width > surfaceCapabilitiesKHR.maxImageExtent.width) {
                swapchainExtent.width = surfaceCapabilitiesKHR.maxImageExtent.width;
            }
            if (swapchainExtent.height < surfaceCapabilitiesKHR.minImageExtent.height) {
                swapchainExtent.height = surfaceCapabilitiesKHR.minImageExtent.height;
            } else if (swapchainExtent.height > surfaceCapabilitiesKHR.maxImageExtent.height) {
                swapchainExtent.height = surfaceCapabilitiesKHR.maxImageExtent.height;
            }
        } else {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent = surfaceCapabilitiesKHR.currentExtent;
        }
        std::cout << "swapchainExtent : " << swapchainExtent.width << " x "
                  << swapchainExtent.height << std::endl;
        auto defaultDevFormatProps = physicalDevice.getFormatProperties(depthFormat);
        auto surfaceDefaultFormat = getSurfaceDefaultFormat(surfaceKHR);

        vk::ImageTiling tiling;
        if (defaultDevFormatProps.linearTilingFeatures &
            vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eLinear;
        } else if (defaultDevFormatProps.optimalTilingFeatures &
                   vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eOptimal;
        } else {
            std::cout << "vk::ImageTiling no match exit!" << std::endl;
            exit(-1);//todo throw
        }
        depthImage = createImageUnique(vk::ImageCreateInfo{vk::ImageCreateFlags(),
                                                           vk::ImageType::e2D,
                                                           depthFormat,
                                                           vk::Extent3D{swapchainExtent.width,
                                                                        swapchainExtent.height, 1},
                                                           1,
                                                           1,
                                                           vk::SampleCountFlagBits::e1,
                                                           tiling,
                                                           vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                           vk::SharingMode::eExclusive,
                                                           0, nullptr,
                                                           vk::ImageLayout::eUndefined});

        depthImageMemory = allocBindImageMemory(depthImage.get(),
                                                vk::MemoryPropertyFlagBits::eDeviceLocal);

        depthImageView = createImageViewUnique(vk::ImageViewCreateInfo{vk::ImageViewCreateFlags(),
                                                                       depthImage.get(),
                                                                       vk::ImageViewType::e2D,
                                                                       surfaceDefaultFormat.format,
                                                                       vk::ComponentMapping{
                                                                               vk::ComponentSwizzle::eR,
                                                                               vk::ComponentSwizzle::eG,
                                                                               vk::ComponentSwizzle::eB,
                                                                               vk::ComponentSwizzle::eA},
                                                                       vk::ImageSubresourceRange{
                                                                               vk::ImageAspectFlagBits::eColor,
                                                                               0, 1, 0, 1}
        });
        vk::SurfaceTransformFlagBitsKHR preTransform{};
        if (surfaceCapabilitiesKHR.supportedTransforms &
            vk::SurfaceTransformFlagBitsKHR::eIdentity) {
            preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        } else {
            preTransform = surfaceCapabilitiesKHR.currentTransform;
        }
        vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
                vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
                vk::CompositeAlphaFlagBitsKHR::eInherit
        };
        for (auto &compositeAlphaFlag:compositeAlphaFlags) {
            if (surfaceCapabilitiesKHR.supportedCompositeAlpha & compositeAlphaFlag) {
                compositeAlpha = compositeAlphaFlag;
                break;
            }
        }
        assert(surfaceCapabilitiesKHR.minImageCount <= SWAPCHAIN_NUM &&
               surfaceCapabilitiesKHR.maxImageCount >= SWAPCHAIN_NUM);
        vk::SwapchainCreateInfoKHR swapChainCreateInfo{vk::SwapchainCreateFlagsKHR(),
                                                       surfaceKHR,
                                                       SWAPCHAIN_NUM,
                                                       surfaceDefaultFormat.format,
                                                       vk::ColorSpaceKHR::eSrgbNonlinear,
                                                       swapchainExtent,
                                                       1,
                                                       vk::ImageUsageFlags{} |
                                                       vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eTransferSrc,
                                                       vk::SharingMode::eExclusive,
                                                       0,//TODO to spt graphics and present queues from different queue families
                                                       nullptr,
                                                       preTransform,
                                                       compositeAlpha,
                                                       vk::PresentModeKHR::eFifo,
                                                       false};
        swapchainKHR = createSwapchainKHRUnique(swapChainCreateInfo);
        auto vkSwapChainImages = getSwapchainImagesKHR(swapchainKHR.get());
        std::cout << "vkSwapChainImages size : " << vkSwapChainImages.size() << std::endl;

        vkSwapChainBuffers.clear();
        for (auto &vkSwapChainImage : vkSwapChainImages) {
            auto imageView = createImageViewUnique(vk::ImageViewCreateInfo{
                    vk::ImageViewCreateFlags(),
                    vkSwapChainImage,
                    vk::ImageViewType::e2D,
                    surfaceDefaultFormat.format,
                    vk::ComponentMapping{
                            vk::ComponentSwizzle::eR,
                            vk::ComponentSwizzle::eG,
                            vk::ComponentSwizzle::eB,
                            vk::ComponentSwizzle::eA},
                    vk::ImageSubresourceRange{
                            vk::ImageAspectFlagBits::eColor,
                            0, 1, 0, 1}
            });
            vk::ImageView attachments[2]{imageView.get(), depthImageView.get()};
            auto frameBuffer = createFramebufferUnique(vk::FramebufferCreateInfo{
                    vk::FramebufferCreateFlags(),
                    renderPass.get(),
                    2, attachments,
                    swapchainExtent.width, swapchainExtent.height,
                    1
            });
            vkSwapChainBuffers.emplace_back(vkSwapChainImage, std::move(imageView),
                                            std::move(frameBuffer),
                                            createFenceUnique(vk::FenceCreateInfo{}));
        }
    }


    void Device::buildMVPBufferAndWrite(glm::mat4 MVP) {
        mvpBuffer = createBufferUnique(
                vk::BufferCreateInfo{
                        vk::BufferCreateFlags(),
                        sizeof(MVP),
                        vk::BufferUsageFlagBits::eUniformBuffer});
        auto mvpBufferMemoryRq = getBufferMemoryRequirements(mvpBuffer.get());
        uint32_t typeIndex = findMemoryTypeIndex(mvpBufferMemoryRq.memoryTypeBits,
                                                 vk::MemoryPropertyFlags() |
                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                 vk::MemoryPropertyFlagBits::eHostCoherent);

        mvpMemory = allocateMemoryUnique(vk::MemoryAllocateInfo{
                mvpBufferMemoryRq.size, typeIndex
        });
        std::cout << "mvpMemory:alloc index:" << typeIndex << std::endl;

        memcpy(mapMemory(mvpMemory.get(), 0, mvpBufferMemoryRq.size,
                         vk::MemoryMapFlagBits()), &MVP, sizeof(MVP));

        unmapMemory(mvpMemory.get());
        auto mvpBufferInfo = vk::DescriptorBufferInfo{mvpBuffer.get(), 0, sizeof(MVP)};
        bindBufferMemory(mvpBuffer.get(), mvpMemory.get(), 0);
        updateDescriptorSets(
                std::vector<vk::WriteDescriptorSet>{
                        vk::WriteDescriptorSet{descriptorSets[0].get(), 0, 0, 1,
                                               vk::DescriptorType::eUniformBuffer,
                                               nullptr, &mvpBufferInfo}},
                nullptr);    //todo use_texture

    }

    void Device::updateMVPBuffer(glm::mat4 MVP) {
        if (mvpBuffer && mvpMemory) {
            auto mvpBufferMemoryRq = getBufferMemoryRequirements(mvpBuffer.get());
            memcpy(mapMemory(mvpMemory.get(), 0, mvpBufferMemoryRq.size,
                             vk::MemoryMapFlagBits()), &MVP, sizeof(MVP));
            unmapMemory(mvpMemory.get());
        } else return buildMVPBufferAndWrite(MVP);
    }

    void Device::buildRenderpass(vk::SurfaceKHR &surfaceKHR) {
        auto surfaceDefaultFormat = getSurfaceDefaultFormat(surfaceKHR);
        std::array<vk::AttachmentDescription, 2> attachDescs{
                vk::AttachmentDescription{
                        vk::AttachmentDescriptionFlags(),
                        surfaceDefaultFormat.format,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::ePresentSrcKHR
                },
                vk::AttachmentDescription{
                        vk::AttachmentDescriptionFlags(),
                        depthFormat,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::AttachmentLoadOp::eLoad,
                        vk::AttachmentStoreOp::eStore,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eColorAttachmentOptimal
                }
        };
        std::array<vk::AttachmentReference, 1> attachmentRefs{
                vk::AttachmentReference{
                        0, vk::ImageLayout::eColorAttachmentOptimal
                }
        };
        vk::AttachmentReference depthAttacheRefs{
                1, vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        std::array<vk::SubpassDescription, 1> subpassDescs{
                vk::SubpassDescription{
                        vk::SubpassDescriptionFlags(),
                        vk::PipelineBindPoint::eGraphics,
                        0, nullptr,
                        attachmentRefs.size(), attachmentRefs.data(),
                        nullptr,
                        &depthAttacheRefs,
                }
        };
        renderPass = createRenderPassUnique(vk::RenderPassCreateInfo{
                vk::RenderPassCreateFlags(),
                attachDescs.size(), attachDescs.data(),
                subpassDescs.size(), subpassDescs.data()
        });

    }

    void Device::buildPipeline(uint32_t dataStepSize) {
        if (graphicsPipeline)
            return;
        static auto vertShaderSpirv = GLSLtoSPV(vk::ShaderStageFlagBits::eVertex, vertShaderText);
        std::cout << "vertShaderSpirv len:" << vertShaderSpirv.size() << 'x'
                  << sizeof(decltype(vertShaderSpirv)
        ::value_type)<<std::endl;
        static auto fargShaderSpirv = GLSLtoSPV(vk::ShaderStageFlagBits::eFragment, fragShaderText);
        std::cout << "fargShaderSpirv len:" << fargShaderSpirv.size() << 'x'
                  << sizeof(decltype(fargShaderSpirv)
        ::value_type)<<std::endl;
        static auto vertShaderModule = createShaderModuleUnique(vk::ShaderModuleCreateInfo{
                vk::ShaderModuleCreateFlags(), vertShaderSpirv.size() *
                                               sizeof(decltype(vertShaderSpirv)::value_type), vertShaderSpirv.data()
        });
        static auto fargShaderModule = createShaderModuleUnique(vk::ShaderModuleCreateInfo{
                vk::ShaderModuleCreateFlags(), fargShaderSpirv.size() *
                                               sizeof(decltype(fargShaderSpirv)::value_type), fargShaderSpirv.data()
        });
        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStageCreateInfos{
                vk::PipelineShaderStageCreateInfo{
                        vk::PipelineShaderStageCreateFlags(),
                        vk::ShaderStageFlagBits::eVertex,
                        vertShaderModule.get(), "main"
                },
                vk::PipelineShaderStageCreateInfo{
                        vk::PipelineShaderStageCreateFlags(),
                        vk::ShaderStageFlagBits::eFragment,
                        fargShaderModule.get(), "main"
                }
        };

        std::array<vk::VertexInputBindingDescription, 1> vertexInputBindingDescriptions{
                vk::VertexInputBindingDescription{
                        0, dataStepSize,// sizeof(g_vb_solid_face_colors_Data[0]),
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
                0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
                vk::FrontFace::eClockwise, 0,
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
                pipelineLayout.get(),
                renderPass.get()
        };
        graphicsPipeline = createGraphicsPipelineUnique(vkPipelineCache.get(), pipelineCreateInfo);
    }

    uint32_t Device::drawCmdBuffer(vk::CommandBuffer &cmdBuffer, vk::Buffer vertexBuffer) {
        {
            std::unique_lock<std::mutex> lockFrame{mutexDraw};
            cvDraw.wait(lockFrame, [this]() { return frameSubmitIndex.size() < 2||submitExitFlag; });
            if(submitExitFlag)
                return 0;
        }
        auto imageAcquiredSemaphore = createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        //auto acquireNextImageFence = createFenceUnique(vk::FenceCreateInfo{});
        //std::cout << "acquireNextImageKHR:" << std::endl;
        auto currentBufferIndex = acquireNextImageKHR(swapchainKHR.get(), UINT64_MAX,
                                                      imageAcquiredSemaphore.get(),
                                                      vk::Fence{});
        //std::cout << "acquireNextImageKHR:" << vk::to_string(currentBufferIndex.result)
        //          << currentBufferIndex.value << std::endl;
        static std::array<vk::ClearValue, 2> clearValues{
                vk::ClearColorValue{std::array<float, 4>{0.5f, 0.2f, 0.2f, 0.2f}},
                vk::ClearDepthStencilValue{1.0f, 0},
        };
        cmdBuffer.begin(vk::CommandBufferBeginInfo{});

        cmdBuffer.beginRenderPass(vk::RenderPassBeginInfo{
                renderPass.get(),
                std::get<vk::UniqueFramebuffer>(vkSwapChainBuffers[currentBufferIndex.value]).get(),
                vk::Rect2D{vk::Offset2D{}, swapchainExtent},
                clearValues.size(), clearValues.data()
        }, vk::SubpassContents::eInline);
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());
        std::array<vk::DescriptorSet, 1> tmpDescriptorSets{this->descriptorSets[0].get()};
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0,
                                     tmpDescriptorSets, std::vector<uint32_t>{});
        vk::DeviceSize offsets[1] = {0};
        cmdBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
        cmdBuffer.draw(12 * 3, 1, 0, 0);
        cmdBuffer.endRenderPass();
        cmdBuffer.end();
        vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        std::array<vk::SubmitInfo, 1> submitInfos{
                vk::SubmitInfo{
                        1, &imageAcquiredSemaphore.get(), &pipelineStageFlags,
                        1, &cmdBuffer
                }
        };
        auto vk_queue = getQueue(queueFamilyIndex, 0);
        //getFenceFdKHR(vk::FenceGetFdInfoKHR{});
        vk_queue.submit(submitInfos, std::get<vk::UniqueFence>(
                vkSwapChainBuffers[currentBufferIndex.value]).get());
         currentBufferIndex.value;
        //std::cout << "push index:" << currentBufferIndex.value << std::endl;

        {
            std::lock_guard<std::mutex> lock{mutexDraw};
            frameSubmitIndex.push(currentBufferIndex.value);
        }
        return 1;
    }


    void Device::buildSubmitThread(vk::SurfaceKHR &surfaceKHR) {
        submitExitFlag = false;
        submitThread = std::make_unique<std::thread>([this, &surfaceKHR] {
            try {
                while (draw_run(*this, surfaceKHR));
            }
            catch (std::system_error systemError) {
                std::cout << "got system error:" << systemError.what() << "!#" << systemError.code()
                          << std::endl;
            }
            catch (std::logic_error logicError) {
                std::cout << "got logic error:" << logicError.what() << std::endl;
            }
        });
    }

    void Device::stopSubmitThread() {
        {
            std::lock_guard<std::mutex> lock{mutexDraw};
            submitExitFlag = true;
        }
        cvDraw.notify_all();
        submitThread->join();
    }

    void Device::swapchainPresent() {

        if (frameSubmitIndex.empty()) {
            cvDraw.notify_all();
            return;
        }
        uint32_t index = frameSubmitIndex.front();
        auto waitRet = waitForFences(1, std::get<vk::UniqueFence>(
                vkSwapChainBuffers[index]).operator->(), true, 100000000);
        if (waitRet != vk::Result::eSuccess) {
            std::lock_guard<std::mutex> lock{mutexDraw};
            std::cout << "waitForFences ret:" << vk::to_string(waitRet) << std::endl;
            //todo fix timeout
            frameSubmitIndex.pop();
            return;
        }
        {
            std::lock_guard<std::mutex> lock{mutexDraw};
            frameSubmitIndex.pop();
            auto presentRet = getQueue(queueFamilyIndex, 0).presentKHR(vk::PresentInfoKHR{
                    0, nullptr, 1, &swapchainKHR.get(), &index
            });
        }
        //std::cout << "frameSubmitIndex pop :" << frameSubmitIndex.size() <<" idx :"<<index<< std::endl;

        if(frameSubmitIndex.size()<2)
            cvDraw.notify_all();

    }
}