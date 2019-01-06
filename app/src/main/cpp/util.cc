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
namespace tt {
    tt::Instance createInstance() {
        auto instanceLayerProperties = vk::enumerateInstanceLayerProperties();
        std::cout << "enumerateInstanceLayerProperties:" << instanceLayerProperties.size()
                  << std::endl;
        std::vector<const char *> instanceLayerPropertiesName;

        for (auto &prop:instanceLayerProperties){
            std::cout << prop.layerName << std::endl;
            //instanceLayerPropertiesName.emplace_back(prop.layerName);
        }
        vk::ApplicationInfo vkAppInfo{"stbox", VK_VERSION_1_0, "stbox",
                                      VK_VERSION_1_0, VK_API_VERSION_1_0};
        std::array<const char *, 2> instanceEtensionNames{VK_KHR_SURFACE_EXTENSION_NAME,
                                                          VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};

        vk::InstanceCreateInfo instanceInfo{vk::InstanceCreateFlags(), &vkAppInfo,
                                            instanceLayerPropertiesName.size(),
                                            instanceLayerPropertiesName.data(),
                                            instanceEtensionNames.size(),
                                            instanceEtensionNames.data()};
        return tt::Instance{vk::createInstanceUnique(instanceInfo)};
    }

    uint32_t queueFamilyPropertiesFindFlags(vk::PhysicalDevice PhyDevice, vk::QueueFlags flags,
                                            vk::SurfaceKHR surface) {
        auto queueFamilyProperties = PhyDevice.getQueueFamilyProperties();
        //std::cout << "getQueueFamilyProperties size : "
        //          << queueFamilyProperties.size() << std::endl;
        for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
            //std::cout << "QueueFamilyProperties : " << i << "\tflags:"
            //          << vk::to_string(queueFamilyProperties[i].queueFlags) << std::endl;
            if (PhyDevice.getSurfaceSupportKHR(i, surface) &&
                (queueFamilyProperties[i].queueFlags & flags)) {
                std::cout << "default_queue_index :" << i << "\tgetSurfaceSupportKHR:true"
                          << std::endl;
                return i;
            }
        }
        throw std::logic_error{"queueFamilyPropertiesFindFlags Error"};
    }



    std::unique_ptr<tt::Device> Instance::connectToDevice(vk::PhysicalDevice& phyDevice,int queueIndex) {
        std::array<float, 1> queue_priorities{0.0};
        std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos{
                vk::DeviceQueueCreateInfo{vk::DeviceQueueCreateFlags(),
                                          queueIndex,
                                          queue_priorities.size(), queue_priorities.data()
                }
        };

        auto deviceLayerProperties = phyDevice.enumerateDeviceLayerProperties();
        std::cout << "phyDeviceDeviceLayerProperties : " << deviceLayerProperties.size() <<std::endl;
        std::vector<const char *> deviceLayerPropertiesName;
        for(auto deviceLayerPropertie :deviceLayerProperties) {
            std::cout << "phyDeviceDeviceLayerPropertie : " << deviceLayerPropertie.layerName
                      << std::endl;
            deviceLayerPropertiesName.emplace_back(deviceLayerPropertie.layerName);
        }
        auto deviceExtensionProperties = phyDevice.enumerateDeviceExtensionProperties();
        for (auto &deviceExtensionPropertie:deviceExtensionProperties)
            std::cout << "PhyDeviceExtensionPropertie : " << deviceExtensionPropertie.extensionName
                      << std::endl;
        std::array<const char *, 1> device_extension_names{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        return std::make_unique<Device>(phyDevice.createDeviceUnique(
                vk::DeviceCreateInfo {vk::DeviceCreateFlags(),
                                      device_queue_create_infos.size(),
                                      device_queue_create_infos.data(),
                                      deviceLayerPropertiesName.size(),
                                      deviceLayerPropertiesName.data(),
                                      device_extension_names.size(),
                                      device_extension_names.data()}),
                                        phyDevice,
                                        queueIndex);
    }


    uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice,uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags) {
        auto memoryProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((memoryTypeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
                    std::cout<<vk::to_string(memoryProperties.memoryTypes[i].propertyFlags)<<" march " << vk::to_string(flags) <<" return "<< i<< std::endl;
                    return i;
                }
            }
            memoryTypeBits >>= 1;
        }
        std::cout << "memoryProperties no match exit!" << std::endl;
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
            std::cout << fileName <<std::endl;
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
                        0, dataStepSize,
                        vk::VertexInputRate::eVertex
                }
        };
        std::array<vk::VertexInputAttributeDescription, 2> vertexInputAttributeDescriptions{
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
        std::array<vk::DynamicState ,2> dynamicStates{vk::DynamicState::eViewport,vk::DynamicState::eScissor};
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
        std::array<vk::AttachmentDescription, 2> attachDescs{
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
        std::array<vk::SubpassDependency, 2> subpassDeps{
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
    Device::createCmdBuffers(tt::Swapchain &swapchain,
                             std::function<void(RenderpassBeginHandle&)> functionRenderpassBegin,
                             std::function<void(CommandBufferBeginHandle&)> functionBegin) {
        std::cout<<__func__<<":allocateCommandBuffersUnique:"<<swapchain.getFrameBufferNum()<<std::endl;
        std::vector<vk::UniqueCommandBuffer> commandBuffers = get().allocateCommandBuffersUnique(
                vk::CommandBufferAllocateInfo{commandPool.get(),
                                              vk::CommandBufferLevel::ePrimary, swapchain.getFrameBufferNum()});

        std::array<vk::ClearValue, 2> clearValues{
                vk::ClearColorValue{std::array<float, 4>{0.5f, 0.2f, 0.2f, 0.2f}},
                vk::ClearDepthStencilValue{1.0f, 0},
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
        std::cout<<__func__<<":allocateCommandBuffersUnique:"<<cmdNum<<std::endl;
        std::vector<vk::UniqueCommandBuffer> commandBuffers = get().allocateCommandBuffersUnique(
                vk::CommandBufferAllocateInfo{commandPool.get(),
                                              vk::CommandBufferLevel::ePrimary, cmdNum});
        for(auto&cmdBuffer : commandBuffers){
            CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
            functionBegin(cmdBeginHandle);
        }
        return commandBuffers;
    }

    vk::UniqueFence Device::submitCmdBuffer(Swapchain &swapchain,
                                            std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers,
                                            vk::Semaphore &imageAcquiredSemaphore,
                                            vk::Semaphore &renderSemaphore) {
        auto currentBufferIndex = get().acquireNextImageKHR(swapchain.get(), UINT64_MAX,
                                                            imageAcquiredSemaphore, nullptr);

        vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        std::array<vk::SubmitInfo, 1> submitInfos{
                vk::SubmitInfo{
                        1, &imageAcquiredSemaphore, &pipelineStageFlags,
                        1, &drawcommandBuffers[currentBufferIndex.value].get(),
                        1, &renderSemaphore
                }
        };
        auto renderFence = get().createFenceUnique(vk::FenceCreateInfo{});
        get().getQueue(queueFamilyIndex, 0).submit(submitInfos, renderFence.get());
        auto presentRet = get().getQueue(queueFamilyIndex, 0).presentKHR(vk::PresentInfoKHR{
                1, &renderSemaphore, 1, &swapchain.get(),&currentBufferIndex.value});
        std::cout << "index:" << currentBufferIndex.value<<"\tpresentRet:"<<vk::to_string(presentRet)<< std::endl;
        return renderFence;
    }

    void Device::submitCmdBufferAndWait(Swapchain &swapchain,
                                        std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers) {
        auto imageAcquiredSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        auto renderSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        auto renderFence = submitCmdBuffer(swapchain,drawcommandBuffers,imageAcquiredSemaphore.get(),renderSemaphore.get());
        waitFence(renderFence.get());
        //wait renderFence then free renderSemaphore imageAcquiredSemaphore
    }

    Device::ImageViewMemory Device::createImageAndMemory(vk::Format format, vk::Extent3D extent3D,
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
                imageMemoryRq.size, findMemoryTypeIndex(physicalDevice,imageMemoryRq.memoryTypeBits,
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

    Device::BufferViewMemory
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
        auto typeIndex = findMemoryTypeIndex(physicalDevice,memoryRequirements.memoryTypeBits,
                                             memoryPropertyFlags);
        std::get<vk::UniqueDeviceMemory>(BVM) = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
                memoryRequirements.size, typeIndex
        });
        std::get<size_t>(BVM) = memoryRequirements.size;
        get().bindBufferMemory(std::get<vk::UniqueBuffer>(BVM).get(),
                         std::get<vk::UniqueDeviceMemory>(BVM).get(), 0);
        return BVM;
    }




    Swapchain::Swapchain(vk::UniqueSurfaceKHR &&sf, tt::Device &device,vk::Extent2D windowExtent)
            : surface{std::move(sf)} ,swapchainExtent{windowExtent}{
        auto physicalDevice = device.phyDevice();
        auto surfaceCapabilitiesKHR = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
        //std::cout << "AndroidGetWindowSize() : " << swapchainExtent.width << " x "
        //          << swapchainExtent.height << std::endl;
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
        auto surfacePresentMods = physicalDevice.getSurfacePresentModesKHR(surface.get());

        //if(std::find(surfacePresentMods.begin(),surfacePresentMods.end(),vk::PresentModeKHR::eMailbox) != surfacePresentMods.end()){
        //    std::cout << "surfacePresentMods have: eMailbox\n";
        //}
        for (auto &surfacePresentMod :surfacePresentMods) {
            std::cout << "\t\tsurfacePresentMods have " << vk::to_string(surfacePresentMod)
                      << std::endl;
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
        vk::UniqueSwapchainKHR::operator=(device->createSwapchainKHRUnique(swapChainCreateInfo));

        auto vkSwapChainImages = device->getSwapchainImagesKHR(get());
        std::cout << "vkSwapChainImages size : " << vkSwapChainImages.size() << std::endl;

        imageViews.clear();
        for (auto &vkSwapChainImage : vkSwapChainImages)
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
            std::array<vk::ImageView, 2> attachments{imageView.get(),
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