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

	void stboxvk::initDevice(android_app *app, tt::Instance &instance,
	                         vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR surface) {
		devicePtr = instance.connectToDevice(physicalDevice, surface);//reconnect
		auto defaultDeviceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
		for (auto &phdFormat:defaultDeviceFormats) {
			MY_LOG(INFO) << vk::to_string(phdFormat.colorSpace) << "@"
			             << vk::to_string(phdFormat.format);
		}
		devicePtr->renderPass = devicePtr->createRenderpass(defaultDeviceFormats[0].format);
	}

	void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
		assert(instance);
		//initData(app,instance);
		auto surface = instance.connectToWSI(app->window);
		if (!devicePtr || !devicePtr->checkSurfaceSupport(surface.get())) {
			auto phyDevices = instance->enumeratePhysicalDevices();
			//phyDevices[0].getSurfaceCapabilities2KHR(vk::PhysicalDeviceSurfaceInfo2KHR{surface.get()});
			//auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(phyDevices[0],
			//                                                         vk::QueueFlagBits::eGraphics,
			//                                                         surface.get());
			initDevice(app, instance, phyDevices[0], surface.get());
		}
		auto &job = initJobs(app, *devicePtr);

		windowPtr = std::make_unique<tt::Window>(std::move(surface), *devicePtr,
		                                         AndroidGetWindowSize(app));

		auto pv = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(windowPtr->getSwapchainExtent().width) /
				static_cast<float>(windowPtr->getSwapchainExtent().height),
				0.1f, 256.0f) *
		          glm::lookAt(
				          glm::vec3(8, 3, 10),  // Camera is at (-5,3,-10), in World Space
				          glm::vec3(0, 0, 0),     // and looks at the origin
				          glm::vec3(0, 1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
		          );
		job.writeBvm(0, &pv, sizeof(pv));
		job.buildCmdBuffer(
				*windowPtr,
				[&](RenderpassBeginHandle &cmdHandleRenderpassBegin) {
					std::array viewports{
							vk::Viewport{
									0, 0,
									windowPtr->getSwapchainExtent().width,
									windowPtr->getSwapchainExtent().height,
									0.0f, 1.0f
							}
					};
					cmdHandleRenderpassBegin.setViewport(0, viewports);
					std::array scissors{
							vk::Rect2D{vk::Offset2D{}, windowPtr->getSwapchainExtent()}
					};
					cmdHandleRenderpassBegin.setScissor(0, scissors);

					cmdHandleRenderpassBegin.bindPipeline(
							vk::PipelineBindPoint::eGraphics,
							job.uniquePipeline.get());
					std::array tmpDescriptorSets{job.descriptorSets[0].get()};
					cmdHandleRenderpassBegin.bindDescriptorSets(
							vk::PipelineBindPoint::eGraphics,
							job.pipelineLayout.get(), 0,
							tmpDescriptorSets,
							std::vector<uint32_t>{}
					);
					vk::DeviceSize offsets[1] = {0};
					cmdHandleRenderpassBegin.bindVertexBuffers(
							0, 1,
							&std::get<vk::UniqueBuffer>(
									job.BVMs[1]).get(),
							offsets
					);
					cmdHandleRenderpassBegin.bindIndexBuffer(
							std::get<vk::UniqueBuffer>(job.BVMs[2]).get(),
							0, vk::IndexType::eUint32
					);
					cmdHandleRenderpassBegin.drawIndexed(6, 1, 0, 0, 0);
				});

		return devicePtr->runJobOnWindow(job, *windowPtr);

	}

	Job &stboxvk::initJobs(android_app *app, tt::Device &device) {
		if(!device.jobs.empty())
			return device.jobs[0];
		auto &job = device.createJob(
				std::vector{
						vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
						vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2},
						vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2}
				},
				std::vector{
						vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer,
						                               1, vk::ShaderStageFlagBits::eVertex},
						vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler,
						                               1, vk::ShaderStageFlagBits::eFragment}
				}
		);

		job.BVMs.emplace_back(
				device.createBufferAndMemory(
						sizeof(glm::mat4),
						vk::BufferUsageFlagBits::eUniformBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		std::vector<VertexUV> vertices{
				{{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
				{{-1.0f, 1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
				{{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		};
		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						vertices, vk::BufferUsageFlagBits::eVertexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						std::vector<uint32_t>{0, 1, 2, 2, 3, 0},
						vk::BufferUsageFlagBits::eIndexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));


		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);

		job.uniquePipeline = device.createPipeline(
				sizeof(decltype(vertices)::value_type),
				std::vector{
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
				},
				job.pipelineCache.get(),
				job.pipelineLayout.get()
		);


		{
			auto fileContent = loadDataFromAssets("textures/vulkan_11_rgba.ktx", app);
			gli::texture2d tex2d;
			tex2d = gli::texture2d{gli::load(fileContent.data(), fileContent.size())};
			job.sampler = device.createSampler(tex2d.levels());
			job.IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));

		}

		auto descriptorBufferInfo = device.getDescriptorBufferInfo(job.BVMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(job.IVMs[0], job.sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(), 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device.get().updateDescriptorSets(writeDes, nullptr);

		return job;
	}

	void stboxvk::cleanWindow() {
		//MY_LOG(INFO) << __func__ ;
		windowPtr.reset();
		//devicePtr.reset();
	}


}
