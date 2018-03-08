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
#include <map>

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

    Device Instance::connectToDevice() {
        auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(vk::QueueFlagBits::eGraphics);
        std::array<float, 1> queue_priorities{0.0};
        std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos{
                vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(),
                                          graphicsQueueIndex,
                                          queue_priorities.size(), queue_priorities.data()
                }};
        std::array<const char *, 1> device_extension_names{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        return Device{defaultPhyDevice().createDevice(
                vk::DeviceCreateInfo {vk::DeviceCreateFlags(),
                                      device_queue_create_infos.size(),
                                      device_queue_create_infos.data(),
                                      0, nullptr, device_extension_names.size(),
                                      device_extension_names.data()}),
                      defaultPhyDevice(),
                      graphicsQueueIndex};
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

    std::vector<vk::CommandBuffer>
    Device::defaultPoolAllocBuffer(vk::CommandBufferLevel bufferLevel, uint32_t num) {
        return allocateCommandBuffers(
                vk::CommandBufferAllocateInfo{commandPool.get(), bufferLevel, num});
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
    Device::AllocBindImageMemory(vk::Image image, vk::MemoryPropertyFlags flags) {
        auto imageMemoryRq = getImageMemoryRequirements(image);
        uint32_t typeIndex = findMemoryTypeIndex(imageMemoryRq.memoryTypeBits, flags);
        auto imageMemory = allocateMemoryUnique(vk::MemoryAllocateInfo{
                imageMemoryRq.size, findMemoryTypeIndex(imageMemoryRq.memoryTypeBits, flags)
        });
        std::cout << "ImageMemory:alloc index:" << typeIndex << std::endl;
        bindImageMemory(image, imageMemory.get(), 0);
        return imageMemory;
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
        auto surfaceDefaultFormat = getSurfaceDefaultFormat(surfaceKHR);
        vk::SwapchainCreateInfoKHR swapChainCreateInfo{vk::SwapchainCreateFlagsKHR(),
                                                          surfaceKHR,
                                                          surfaceCapabilitiesKHR.minImageCount,
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
            vkSwapChainBuffers.emplace_back(vkSwapChainImage,
                                            createImageViewUnique(vk::ImageViewCreateInfo{
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
                                            })
            );
        }
        auto vkDefaultDevFormatProps = physicalDevice.getFormatProperties(depthFormat);
        vk::ImageTiling tiling;
        if (vkDefaultDevFormatProps.linearTilingFeatures &
            vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eLinear;
        } else if (vkDefaultDevFormatProps.optimalTilingFeatures &
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

        depthImageMemory = AllocBindImageMemory(depthImage.get(),
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
        auto mvpBufferInfo=vk::DescriptorBufferInfo{mvpBuffer.get(), 0, sizeof(MVP)};
        bindBufferMemory(mvpBuffer.get(), mvpMemory.get(), 0);
        updateDescriptorSets(
                std::vector<vk::WriteDescriptorSet>{vk::WriteDescriptorSet{descriptorSets[0].get(), 0, 0, 1,
                                                                           vk::DescriptorType::eUniformBuffer,
                                                                           nullptr, &mvpBufferInfo}},
                nullptr);    //todo use_texture

    }

    void Device::buildRenderpass(vk::SurfaceKHR &surfaceKHR){
        auto surfaceDefaultFormat = getSurfaceDefaultFormat(surfaceKHR);
        std::array<vk::AttachmentDescription, 2> attach_descs{
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
        std::array<vk::AttachmentReference, 1> attachment_refs{
                vk::AttachmentReference{
                        0, vk::ImageLayout::eColorAttachmentOptimal
                }
        };
        vk::AttachmentReference depth_attache_refs{
                1, vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        std::array<vk::SubpassDescription, 1> subpassDescs{
                vk::SubpassDescription{
                        vk::SubpassDescriptionFlags(),
                        vk::PipelineBindPoint::eGraphics,
                        0, nullptr,
                        attachment_refs.size(), attachment_refs.data(),
                        nullptr,
                        &depth_attache_refs,
                }
        };
        renderPass = createRenderPassUnique(vk::RenderPassCreateInfo{
                vk::RenderPassCreateFlags(),
                attach_descs.size(), attach_descs.data(),
                subpassDescs.size(), subpassDescs.data()
        });

        for (auto &vkSwapChainBuffer : vkSwapChainBuffers) {
            vk::ImageView attachments[2]{vkSwapChainBuffer.second.get(), depthImageView.get()};
            frameBuffers.emplace_back(createFramebufferUnique(vk::FramebufferCreateInfo{
                    vk::FramebufferCreateFlags(),
                    renderPass.get(),
                    2, attachments,
                    swapchainExtent.width, swapchainExtent.height,
                    1
            }));
        }

    }
}