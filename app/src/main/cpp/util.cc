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
//#include "shaderc/shaderc.hpp"
#include "util.hh"
#include "stboxvk.hh"
#include <map>

#include "vertexdata.hh"
#include <algorithm>
#include <cstring>
#include <array>

#include "stb_font_consolas_24_latin1.inl"

using namespace std::chrono_literals;

/*
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
*/

#include <android/log.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT msgFlags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location,
        int32_t msgCode, const char * pLayerPrefix,
        const char * pMsg, void * pUserData )
{
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        __android_log_print(ANDROID_LOG_ERROR,
                            "Stbox",
                            "ERROR: [%s] Code %i : %s",
                            pLayerPrefix, msgCode, pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        __android_log_print(ANDROID_LOG_WARN,
                            "Stbox",
                            "WARNING: [%s] Code %i : %s",
                            pLayerPrefix, msgCode, pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        __android_log_print(ANDROID_LOG_WARN,
                            "Stbox",
                            "PERFORMANCE WARNING: [%s] Code %i : %s",
                            pLayerPrefix, msgCode, pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        __android_log_print(ANDROID_LOG_INFO,
                            "Stbox", "INFO: [%s] Code %i : %s",
                            pLayerPrefix, msgCode, pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        __android_log_print(ANDROID_LOG_VERBOSE,
                            "Stbox", "DEBUG: [%s] Code %i : %s",
                            pLayerPrefix, msgCode, pMsg);
    }

    // Returning false tells the layer not to stop when the event occurs, so
    // they see the same behavior with and without validation layers enabled.
    return VK_FALSE;
}
VkDebugReportCallbackEXT debugReportCallback;


namespace tt {
    tt::Instance createInstance() {
#ifdef __arm__
        MY_LOG(INFO) << "__arm__";
#elif defined __aarch64__
        MY_LOG(INFO) << "__aarch64__";
#endif
        auto instanceLayerProperties = vk::enumerateInstanceLayerProperties();
        MY_LOG(INFO) << "enumerateInstanceLayerProperties:" << instanceLayerProperties.size();
        std::vector<const char *> instanceLayerPropertiesName;

        for (auto &prop:instanceLayerProperties){
            MY_LOG(INFO) << prop.layerName ;
            //instanceLayerPropertiesName.emplace_back(prop.layerName);
        }
        vk::ApplicationInfo vkAppInfo{"stbox", VK_VERSION_1_0, "stbox",
                                      VK_VERSION_1_0, VK_API_VERSION_1_0};

        std::vector instanceEtensionNames{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            //VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        };
        auto instanceExts = vk::enumerateInstanceExtensionProperties();
        for(auto& Ext: instanceExts){
            MY_LOG(INFO) <<"instanceExt"<< Ext.extensionName ;
            if(!std::strcmp(Ext.extensionName,VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
                MY_LOG(INFO) << "push " << Ext.extensionName ;
                instanceEtensionNames.emplace_back(Ext.extensionName);
            }
        }

        vk::InstanceCreateInfo instanceInfo{vk::InstanceCreateFlags(), &vkAppInfo,
                                            instanceLayerPropertiesName.size(),
                                            instanceLayerPropertiesName.data(),
                                            instanceEtensionNames.size(),
                                            instanceEtensionNames.data()};
        auto ins = vk::createInstanceUnique(instanceInfo);
        for(auto& ExtName:instanceEtensionNames)
            if(!std::strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,ExtName)){
                auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)(ins->getProcAddr("vkCreateDebugReportCallbackEXT"));
                vk::DebugReportCallbackCreateInfoEXT debugReportInfo{
                                vk::DebugReportFlagBitsEXT::eError|
                                vk::DebugReportFlagBitsEXT::eWarning|
                                vk::DebugReportFlagBitsEXT::ePerformanceWarning|
                                vk::DebugReportFlagBitsEXT::eDebug|
                                vk::DebugReportFlagBitsEXT::eInformation,
                                DebugReportCallback};
                vkCreateDebugReportCallbackEXT((VkInstance)ins.get(),(VkDebugReportCallbackCreateInfoEXT*)&debugReportInfo, nullptr,&debugReportCallback);
                MY_LOG(ERROR) << "vkCreateDebugReportCallbackEXT" ;
            }


        //vkCreateDebugReportCallbackEXT((VkInstance)ins.get(),(VkDebugReportCallbackCreateInfoEXT*)&debugReportInfo, nullptr,&debugReportCallback);
        //auto vkDestroyDebugReportCallbackEXT = static_cast<PFN_vkDestroyDebugReportCallbackEXT>(ins->getProcAddr("vkDestroyDebugReportCallbackEXT"));
        //assert(vkCreateDebugReportCallbackEXT);
        //assert(vkDestroyDebugReportCallbackEXT);
        return tt::Instance{std::move(ins)};
    }

    uint32_t queueFamilyPropertiesFindFlags(vk::PhysicalDevice PhyDevice, vk::QueueFlags flags,
                                            vk::SurfaceKHR surface) {
        auto queueFamilyProperties = PhyDevice.getQueueFamilyProperties();
        //MY_LOG(INFO) << "getQueueFamilyProperties size : "
        //          << queueFamilyProperties.size() ;
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
            //MY_LOG(INFO) << "QueueFamilyProperties : " << i << "\tflags:"
            //          << vk::to_string(queueFamilyProperties[i].queueFlags) ;
            if (PhyDevice.getSurfaceSupportKHR(i, surface) &&
                (queueFamilyProperties[i].queueFlags & flags)) {
                MY_LOG(INFO) << "default_queue_index :" << i << "\tgetSurfaceSupportKHR:true";
                return i;
            }
        }
        throw std::logic_error{"queueFamilyPropertiesFindFlags Error"};
    }



    std::unique_ptr<tt::Device> Instance::connectToDevice(vk::PhysicalDevice& phyDevice,int queueIndex) {
        std::array<float, 1> queue_priorities{0.0};
        std::array deviceQueueCreateInfos{
                vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(),
                                          queueIndex,
                                          queue_priorities.size(), queue_priorities.data()
                }
        };

        auto deviceLayerProperties = phyDevice.enumerateDeviceLayerProperties();
        //auto deviceFeatures = phyDevice.getFeatures();
        //MY_LOG(INFO) << "deviceFeatures.samplerAnisotropy = "<<deviceFeatures.samplerAnisotropy;
        MY_LOG(INFO) << "phyDeviceDeviceLayerProperties : " << deviceLayerProperties.size() ;
        std::vector<const char *> deviceLayerPropertiesName;
        for(auto &deviceLayerPropertie :deviceLayerProperties) {
            MY_LOG(INFO) << "phyDeviceDeviceLayerPropertie : " << deviceLayerPropertie.layerName;
            deviceLayerPropertiesName.emplace_back(deviceLayerPropertie.layerName);
        }
        auto deviceExtensionProperties = phyDevice.enumerateDeviceExtensionProperties();
        for (auto &deviceExtensionPropertie:deviceExtensionProperties)
            MY_LOG(INFO) << "PhyDeviceExtensionPropertie : " << deviceExtensionPropertie.extensionName;
        std::array deviceExtensionNames{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
            VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,
            //VK_EXT_DEBUG_MARKER_EXTENSION_NAME
            //VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
            //VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME
        };

        return std::make_unique<Device>(phyDevice.createDeviceUnique(
                vk::DeviceCreateInfo {vk::DeviceCreateFlags(),
                                      deviceQueueCreateInfos.size(),
                                      deviceQueueCreateInfos.data(),
                                      deviceLayerPropertiesName.size(),
                                      deviceLayerPropertiesName.data(),
                                      deviceExtensionNames.size(),
                                      deviceExtensionNames.data()}),
                                      phyDevice,
                                      queueIndex);
    }


    uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice,uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags) {
        auto memoryProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((memoryTypeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                    MY_LOG(INFO)<<vk::to_string(memoryProperties.memoryTypes[i].propertyFlags)<<" march " << vk::to_string(flags) <<" return "<< i;
                    return i;
                }

            }
            memoryTypeBits >>= 1;
        }
        MY_LOG(INFO) << "memoryProperties no match exit!" ;
        exit(-1);//todo throw
    }

    std::vector<char> loadDataFromAssets(const std::string& filePath,
                                                        android_app *androidAppCtx) {
        // Read the file
        assert(androidAppCtx);
        std::unique_ptr<AAsset,std::function<void(AAsset *)> > file{
                AAssetManager_open(androidAppCtx->activity->assetManager, filePath.c_str(), AASSET_MODE_STREAMING),
                [](AAsset *AAsset) {
                    AAsset_close(AAsset);
                }
        };
        std::vector<char> fileContent;
        fileContent.resize(AAsset_getLength(file.get()));

        AAsset_read(file.get(), reinterpret_cast<void *>(fileContent.data()), fileContent.size());
        return fileContent;
    }


    StbFontChar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];

    vk::UniqueShaderModule Device::loadShaderFromAssets(const std::string& filePath,
                                                        android_app *androidAppCtx) {
        // Read the file
        auto fileContent = loadDataFromAssets(filePath,androidAppCtx);
        return get().createShaderModuleUnique(vk::ShaderModuleCreateInfo{
                vk::ShaderModuleCreateFlags(), fileContent.size(),
                reinterpret_cast<const uint32_t *>(fileContent.data())});

    }

    std::vector<std::tuple<std::string, vk::UniqueShaderModule>>
    Device::loadShaderFromAssetsDir(const char *dirPath, android_app *androidAppCtx) {
        assert(androidAppCtx);
        std::unique_ptr<AAssetDir,std::function<void(AAssetDir *)> > dir{
                AAssetManager_openDir(androidAppCtx->activity->assetManager, dirPath),
                [](AAssetDir *assetDir) {
                    AAssetDir_close(assetDir);
                }
        };
        std::vector<std::tuple<std::string, vk::UniqueShaderModule>> ShaderModules{};
        for(const char *fileName = AAssetDir_getNextFileName(dir.get());fileName;fileName = AAssetDir_getNextFileName(dir.get())){
            MY_LOG(INFO) << fileName ;
            if(std::strstr(fileName,".comp.spv")){
                std::string fullName{dirPath};
                ShaderModules.emplace_back(fileName,loadShaderFromAssets(fullName+'/'+fileName,androidAppCtx));
            }
        }
        return ShaderModules;
    }

    std::map<std::string,vk::UniquePipeline> Device::createComputePipeline(android_app *app){
        auto shaderModules = loadShaderFromAssetsDir("shaders", app);
        std::map<std::string,vk::UniquePipeline> mapComputePipeline;
        auto descriptorSetLayout = createDescriptorSetLayoutUnique(
                std::vector<vk::DescriptorSetLayoutBinding>{
                        vk::DescriptorSetLayoutBinding{
                                0,
                                vk::DescriptorType::eStorageImage,
                                1,
                                vk::ShaderStageFlagBits::eCompute
                        },
                        vk::DescriptorSetLayoutBinding{
                                1,
                                vk::DescriptorType::eStorageImage,
                                1,
                                vk::ShaderStageFlagBits::eCompute
                        }
                }
        );
        auto pipelineLayout = createPipelineLayout(descriptorSetLayout);

        for(auto& shaderModule:shaderModules){
            vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
                    vk::PipelineShaderStageCreateFlags(),
                    vk::ShaderStageFlagBits::eCompute,
                    std::get<vk::UniqueShaderModule>(shaderModule).get(),
                    "forward"
            };
            vk::ComputePipelineCreateInfo computePipelineCreateInfo{
                vk::PipelineCreateFlags(),
                shaderStageCreateInfo,
                pipelineLayout.get()
            };
            mapComputePipeline.emplace(std::get<std::string>(shaderModule),
                    get().createComputePipelineUnique(pipelineCache.get(),computePipelineCreateInfo));
        }
        return mapComputePipeline;
    };


    vk::UniquePipeline Device::createPipeline(uint32_t dataStepSize, android_app *app,
                                vk::PipelineLayout pipelineLayout) {
        auto vertShaderModule = loadShaderFromAssets("shaders/mvp.vert.spv", app);
        auto fargShaderModule = loadShaderFromAssets("shaders/copy.frag.spv", app);
        std::array shaderStageCreateInfos{
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

        std::array vertexInputBindingDescriptions{
                vk::VertexInputBindingDescription{
                        0, dataStepSize,
                        vk::VertexInputRate::eVertex
                }
        };
        std::array vertexInputAttributeDescriptions{
                vk::VertexInputAttributeDescription{
                        0, 0, vk::Format::eR32G32B32A32Sfloat, 0
                },
                vk::VertexInputAttributeDescription{
                        1, 0, vk::Format::eR32G32Sfloat, 16
                }//VK_FORMAT_R32G32_SFLOAT
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

        vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
                vk::PipelineViewportStateCreateFlags(),
                1, nullptr, 1, nullptr
        };
        std::array dynamicStates{vk::DynamicState::eViewport,vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{vk::PipelineDynamicStateCreateFlags(),dynamicStates.size(),dynamicStates.data()};

        vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
                vk::PipelineRasterizationStateCreateFlags(),
                0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, 0,
                0, 0, 0, 1.0f
        };
        vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
                vk::PipelineDepthStencilStateCreateFlags(), true, true, vk::CompareOp::eLessOrEqual,
                false, false,
                vk::StencilOpState{vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                                   vk::CompareOp::eNever},
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
                vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eClear, 1,
                &pipelineColorBlendAttachmentState
        };
        vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{vk::PipelineMultisampleStateCreateFlags(),vk::SampleCountFlagBits::e1};

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
                pipelineLayout,
                renderPass.get()
        };
        return get().createGraphicsPipelineUnique(pipelineCache.get(), pipelineCreateInfo);
    }

    vk::UniqueRenderPass Device::createRenderpass(vk::Format surfaceDefaultFormat) {
        //auto surfaceDefaultFormat = device.getSurfaceDefaultFormat(surface.get());
        renderPassFormat = surfaceDefaultFormat;
        std::array attachDescs{
                vk::AttachmentDescription{
                        vk::AttachmentDescriptionFlags(),
                        renderPassFormat,
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
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eDepthStencilAttachmentOptimal
                }
        };
        std::array attachmentRefs{
                vk::AttachmentReference{
                        0, vk::ImageLayout::eColorAttachmentOptimal
                }
        };
        vk::AttachmentReference depthAttacheRefs{
                1, vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        std::array subpassDescs{
                vk::SubpassDescription{
                        vk::SubpassDescriptionFlags(),
                        vk::PipelineBindPoint::eGraphics,
                        0, nullptr,
                        attachmentRefs.size(), attachmentRefs.data(),
                        nullptr,
                        &depthAttacheRefs,
                }
        };
        std::array subpassDeps{
                vk::SubpassDependency{
                        VK_SUBPASS_EXTERNAL, 0,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::AccessFlagBits::eMemoryRead,
                        vk::AccessFlagBits::eColorAttachmentRead |
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        vk::DependencyFlagBits::eByRegion
                },
                vk::SubpassDependency{
                        0, VK_SUBPASS_EXTERNAL,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::AccessFlagBits::eColorAttachmentRead |
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        vk::AccessFlagBits::eMemoryRead,
                        vk::DependencyFlagBits::eByRegion
                }
        };
        return get().createRenderPassUnique(vk::RenderPassCreateInfo{
                vk::RenderPassCreateFlags(),
                attachDescs.size(), attachDescs.data(),
                subpassDescs.size(), subpassDescs.data(),
                subpassDeps.size(), subpassDeps.data()
        });
    }

    std::vector<vk::UniqueCommandBuffer>
    Device::createCmdBuffers(tt::Window &swapchain,
                             std::function<void(RenderpassBeginHandle&)> functionRenderpassBegin,
                             std::function<void(CommandBufferBeginHandle&)> functionBegin) {
        MY_LOG(INFO)<<":allocateCommandBuffersUnique:"<<swapchain.getFrameBufferNum();
        std::vector commandBuffers = get().allocateCommandBuffersUnique(
                vk::CommandBufferAllocateInfo{commandPool.get(),
                                              vk::CommandBufferLevel::ePrimary, swapchain.getFrameBufferNum()});

        std::array clearValues{
                vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{0.5f, 0.2f, 0.2f, 0.2f}}},
                vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
        };
        uint32_t frameIndex = 0;
        for(auto&cmdBuffer : commandBuffers){
            //cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
            {
                CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
                functionBegin(cmdBeginHandle);
                {
                    RenderpassBeginHandle cmdHandleRenderpassBegin{
                            cmdBeginHandle,
                            vk::RenderPassBeginInfo{
                                    renderPass.get(),
                                    swapchain.getFrameBuffer()[frameIndex].get(),
                                    vk::Rect2D{
                                            vk::Offset2D{},
                                            swapchain.getSwapchainExtent()
                                    },
                                    clearValues.size(),clearValues.data()
                            }
                    };
                    functionRenderpassBegin(cmdHandleRenderpassBegin);
                }

            }
            ++frameIndex;
        }
        return commandBuffers;
    }

    std::vector<vk::UniqueCommandBuffer>
    Device::createCmdBuffers(size_t cmdNum, std::function<void(CommandBufferBeginHandle &)> functionBegin) {
        MY_LOG(INFO)<<":allocateCommandBuffersUnique:"<<cmdNum;
        std::vector<vk::UniqueCommandBuffer> commandBuffers = get().allocateCommandBuffersUnique(
                vk::CommandBufferAllocateInfo{commandPool.get(),
                                              vk::CommandBufferLevel::ePrimary, cmdNum});
        for(auto&cmdBuffer : commandBuffers){
            CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
            functionBegin(cmdBeginHandle);
        }
        return commandBuffers;
    }

    vk::UniqueFence Window::submitCmdBuffer(Device &device,
                                            std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers,
                                            vk::Semaphore &imageAcquiredSemaphore,
                                            vk::Semaphore &renderSemaphore) {
        auto currentBufferIndex = acquireNextImage(device, imageAcquiredSemaphore);

        vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        std::array submitInfos{
                vk::SubmitInfo{
                        1, &imageAcquiredSemaphore, &pipelineStageFlags,
                        1, &drawcommandBuffers[currentBufferIndex.value].get(),
                        1, &renderSemaphore
                }
        };
        auto renderFence = device->createFenceUnique(vk::FenceCreateInfo{});
        device.graphsQueue().submit(submitInfos, renderFence.get());
        auto presentRet = queuePresent(device.graphsQueue(),currentBufferIndex.value, renderSemaphore);
        MY_LOG(INFO) << "index:" << currentBufferIndex.value<<"\tpresentRet:"<<vk::to_string(presentRet);
        return renderFence;
    }



    ImageViewMemory Device::createImageAndMemory(vk::Format format, vk::Extent3D extent3D,
                                                         vk::ImageUsageFlags imageUsageFlags,
                                                         uint32_t mipLevels,
                                                         vk::ComponentMapping componentMapping,
                                                         vk::ImageSubresourceRange imageSubresourceRange) {
        ImageViewMemory IVM{};
        std::get<vk::UniqueImage>(IVM) = get().createImageUnique(
                vk::ImageCreateInfo{vk::ImageCreateFlags(),
                                    vk::ImageType::e2D,
                                    format,
                                    extent3D,
                                    mipLevels,
                                    1,
                                    vk::SampleCountFlagBits::e1,
                                    vk::ImageTiling::eOptimal,
                                    imageUsageFlags});

        auto imageMemoryRq = get().getImageMemoryRequirements(std::get<vk::UniqueImage>(IVM).get());
        std::get<vk::UniqueDeviceMemory>(IVM) = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
                imageMemoryRq.size, findMemoryTypeIndex(imageMemoryRq.memoryTypeBits,
                                                        vk::MemoryPropertyFlagBits::eDeviceLocal)
        });
        get().bindImageMemory(std::get<vk::UniqueImage>(IVM).get(),
                        std::get<vk::UniqueDeviceMemory>(IVM).get(), 0);

        std::get<vk::UniqueImageView>(IVM) = get().createImageViewUnique(
                vk::ImageViewCreateInfo{vk::ImageViewCreateFlags(),
                                        std::get<vk::UniqueImage>(IVM).get(),
                                        vk::ImageViewType::e2D,
                                        format,
                                        componentMapping,
                                        imageSubresourceRange});
        return IVM;
    }

    BufferViewMemory
    Device::createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
                                  vk::MemoryPropertyFlags memoryPropertyFlags) {

        BufferViewMemory BVM{};
        std::get<vk::UniqueBuffer>(BVM) = get().createBufferUnique(
                vk::BufferCreateInfo{
                        vk::BufferCreateFlags(),
                        dataSize,
                        bufferUsageFlags});
        auto memoryRequirements = get().getBufferMemoryRequirements(
                std::get<vk::UniqueBuffer>(BVM).get());
        auto typeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                             memoryPropertyFlags);
        std::get<vk::UniqueDeviceMemory>(BVM) = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
                memoryRequirements.size, typeIndex
        });
        std::get<size_t>(BVM) = memoryRequirements.size;
        get().bindBufferMemory(std::get<vk::UniqueBuffer>(BVM).get(),
                         std::get<vk::UniqueDeviceMemory>(BVM).get(), 0);
        return BVM;
    }

	Job& Device::createJob(std::vector<vk::DescriptorPoolSize> descriptorPoolSizes,
	                       std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings){
		auto& job = jobs.emplace_back(*this,queueFamilyIndex,std::move(descriptorPoolSizes),std::move(descriptorSetLayoutBindings));

		return job;
	}

	void Window::submitCmdBufferAndWait(Device &device,
	                                    std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers) {
		auto imageAcquiredSemaphore = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		auto renderSemaphore = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		auto renderFence = submitCmdBuffer(device,drawcommandBuffers,imageAcquiredSemaphore.get(),renderSemaphore.get());
		device.waitFence(renderFence.get());
		//wait renderFence then free renderSemaphore imageAcquiredSemaphore
	}

    Window::Window(vk::UniqueSurfaceKHR &&sf, tt::Device &device,vk::Extent2D windowExtent)
            : surface{std::move(sf)} ,swapchainExtent{windowExtent}{
        auto physicalDevice = device.phyDevice();
        auto surfaceCapabilitiesKHR = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
        //MY_LOG(INFO) << "AndroidGetWindowSize() : " << swapchainExtent.width << " x "
        //          << swapchainExtent.height ;
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
        MY_LOG(INFO) << "swapchainExtent : " << swapchainExtent.width << " x "
                  << swapchainExtent.height ;
        auto surfacePresentMods = physicalDevice.getSurfacePresentModesKHR(surface.get());

        //if(std::find(surfacePresentMods.begin(),surfacePresentMods.end(),vk::PresentModeKHR::eMailbox) != surfacePresentMods.end()){
        //    MY_LOG(INFO) << "surfacePresentMods have: eMailbox\n";
        //}
        for (auto &surfacePresentMod :surfacePresentMods) {
            MY_LOG(INFO) << "\t\tsurfacePresentMods had " << vk::to_string(surfacePresentMod);
        }
        uint32_t desiredNumberOfSwapchainImages = surfaceCapabilitiesKHR.minImageCount;
        //auto surfaceDefaultFormat = device.getSurfaceDefaultFormat(surface.get());
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
        assert(surfaceCapabilitiesKHR.minImageCount <= desiredNumberOfSwapchainImages &&
               surfaceCapabilitiesKHR.maxImageCount >= desiredNumberOfSwapchainImages);
        vk::SwapchainCreateInfoKHR swapChainCreateInfo{vk::SwapchainCreateFlagsKHR(),
                                                       surface.get(),
                                                       desiredNumberOfSwapchainImages,
                                                       device.getRenderPassFormat(),
                                                       vk::ColorSpaceKHR::eSrgbNonlinear,
                                                       swapchainExtent,
                                                       1,
                                                       vk::ImageUsageFlagBits::eColorAttachment |
                                                       vk::ImageUsageFlagBits::eTransferSrc,
                                                       vk::SharingMode::eExclusive,
                                                       0,//TODO to spt graphics and present queues from different queue families
                                                       nullptr,
                                                       preTransform,
                                                       compositeAlpha,
                                                       vk::PresentModeKHR::eMailbox,
                                                       true};

        //swap(device->createSwapchainKHRUnique(swapChainCreateInfo));
        swapchain = (device->createSwapchainKHRUnique(swapChainCreateInfo));

        auto swapChainImages = device->getSwapchainImagesKHR(swapchain.get());
        MY_LOG(INFO) << "swapChainImages size : " << swapChainImages.size() ;

        imageViews.clear();
        for (auto &vkSwapChainImage : swapChainImages)
            imageViews.emplace_back(device->createImageViewUnique(vk::ImageViewCreateInfo{
                    vk::ImageViewCreateFlags(),
                    vkSwapChainImage,
                    vk::ImageViewType::e2D,
                    device.getRenderPassFormat(),
                    vk::ComponentMapping{
                            vk::ComponentSwizzle::eR,
                            vk::ComponentSwizzle::eG,
                            vk::ComponentSwizzle::eB,
                            vk::ComponentSwizzle::eA},
                    vk::ImageSubresourceRange{
                            vk::ImageAspectFlagBits::eColor,
                            0, 1, 0, 1}
            }));

        depth = device.createImageAndMemory(device.getDepthFormat(), vk::Extent3D{swapchainExtent.width,
                                                                      swapchainExtent.height, 1});


        frameBuffers.clear();
        for (auto &imageView : imageViews) {
            std::array attachments{imageView.get(),
                                                     std::get<vk::UniqueImageView>(depth).get()};
            frameBuffers.emplace_back(device->createFramebufferUnique(vk::FramebufferCreateInfo{
                    vk::FramebufferCreateFlags(),
                    device.renderPass.get(),
                    attachments.size(), attachments.data(),
                    swapchainExtent.width, swapchainExtent.height,
                    1
            }));
        }

    }

}