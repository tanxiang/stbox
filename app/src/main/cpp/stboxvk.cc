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
	    devicePtr->renderPass = devicePtr->createRenderpass(rederPassFormat);
    }

    void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
        assert(instance);
        //initData(app,instance);
        auto surface = instance.connectToWSI(app->window);
        if (!devicePtr){
            auto phyDevices = instance->enumeratePhysicalDevices();
            //phyDevices[0].getSurfaceCapabilities2KHR(vk::PhysicalDeviceSurfaceInfo2KHR{surface.get()});
            auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(phyDevices[0],
                                                                     vk::QueueFlagBits::eGraphics,
                                                                     surface.get());
            auto defaultDeviceFormats = phyDevices[0].getSurfaceFormatsKHR(surface.get());

            for(auto&phdFormat:defaultDeviceFormats){
                MY_LOG(INFO) << vk::to_string(phdFormat.colorSpace)<<"@"<<vk::to_string(phdFormat.format);
            }
            initDevice(app,instance,phyDevices[0],graphicsQueueIndex,defaultDeviceFormats[0].format);

        }


        windowPtr = std::make_unique<tt::Window>(std::move(surface), *devicePtr, AndroidGetWindowSize(app));


        auto& job = initJobs(app,*devicePtr,*windowPtr);

		return devicePtr->runJobOnWindow(job,*windowPtr);

    }

	Job& stboxvk::initJobs(android_app *app, tt::Device &device, tt::Window& window) {
    	auto& job = device.createJob(
    		std::vector {
    			vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
    			vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2},
    			vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2}
    	    },
			std::vector{
    			vk::DescriptorSetLayoutBinding{0,vk::DescriptorType::eUniformBuffer,1,vk::ShaderStageFlagBits::eVertex},
    			vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
    		}
    	);

		auto Projection =glm::perspective(glm::radians(60.0f), static_cast<float>(window.getSwapchainExtent().width) / static_cast<float>(window.getSwapchainExtent().height), 0.1f, 256.0f);

		auto View = glm::lookAt(
				glm::vec3(8, 3, 10),  // Camera is at (-5,3,-10), in World Space
				glm::vec3(0, 0, 0),     // and looks at the origin
				glm::vec3(0, 1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
		);

		//auto mvpMat4 =  Projection  * View ;
		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromMat(Projection  * View, vk::BufferUsageFlagBits::eUniformBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
						);

		std::vector<VertexUV> vertices{
				{{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
				{{-1.0f, 1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
				{{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		};
		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						vertices, vk::BufferUsageFlagBits::eVertexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						std::vector<uint32_t>{0, 1, 2, 2, 3, 0}, vk::BufferUsageFlagBits::eIndexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

		job.uniquePipeline = device.createPipeline(sizeof(decltype(vertices)::value_type), app,job, job.pipelineLayout.get());


		{
			auto fileContent = loadDataFromAssets("textures/vulkan_11_rgba.ktx", app);
			gli::texture2d tex2d;
			tex2d = gli::texture2d{gli::load(fileContent.data(), fileContent.size())};
			job.sampler = device.createSampler(tex2d.levels());
			job.IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));

		}

		auto descriptorBufferInfo = device.getDescriptorBufferInfo(job.BVMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(job.IVMs[0],job.sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(),0,0,1,vk::DescriptorType::eUniformBuffer,
						nullptr,&descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(),1,0,1,vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device.get().updateDescriptorSets(writeDes, nullptr);

		job.cmdBuffers	= device.createCmdBuffers(window,job.commandPool.get(),[&](RenderpassBeginHandle& cmdHandleRenderpassBegin){
			std::array viewports{
					vk::Viewport {
							0, 0, window.getSwapchainExtent().width, window.getSwapchainExtent().height, 0.0f, 1.0f
					}
			};
			cmdHandleRenderpassBegin.setViewport(0,viewports);
			std::array scissors{
					vk::Rect2D{vk::Offset2D{}, window.getSwapchainExtent()}
			};
			cmdHandleRenderpassBegin.setScissor(0,scissors);

			cmdHandleRenderpassBegin.bindPipeline(vk::PipelineBindPoint::eGraphics, job.uniquePipeline.get());
			std::array tmpDescriptorSets{
					job.descriptorSets[0].get()
			};
			cmdHandleRenderpassBegin.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, job.pipelineLayout.get(), 0,
			                                            tmpDescriptorSets, std::vector<uint32_t>{});
			vk::DeviceSize offsets[1] = {0};
			cmdHandleRenderpassBegin.bindVertexBuffers(0, 1, &std::get<vk::UniqueBuffer>(job.BVMs[1]).get(), offsets);
			cmdHandleRenderpassBegin.bindIndexBuffer(std::get<vk::UniqueBuffer>(job.BVMs[2]).get(),0,vk::IndexType::eUint32);
			cmdHandleRenderpassBegin.drawIndexed(6,1,0,0,0);
		});
		return job;
	}

    void stboxvk::cleanWindow() {
        //MY_LOG(INFO) << __func__ ;
        windowPtr.reset();
        //devicePtr.reset();
    }



}
