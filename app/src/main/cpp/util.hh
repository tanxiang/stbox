//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_UTIL_H
#define STBOX_UTIL_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unistd.h>
#include <vulkan.hpp>
#include <thread>
#include <queue>
#include <optional>
#include <condition_variable>
#include <android_native_app_glue.h>
#include <iostream>
#include <memory>
#include <map>
#include <gli/texture2d.hpp>
#include "main.hh"

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

#define SWAPCHAIN_NUM 2

namespace tt {

	class Device;

	class Instance;

	std::vector<char> loadDataFromAssets(const std::string &filePath, android_app *androidAppCtx);

	inline uint32_t
	queueFamilyPropertiesFindFlags(vk::PhysicalDevice phyDevice, vk::QueueFlags flags,
	                               vk::SurfaceKHR surface) {
		auto queueFamilyProperties = phyDevice.getQueueFamilyProperties();
		//MY_LOG(INFO) << "getQueueFamilyProperties size : "
		//          << queueFamilyProperties.size() ;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
			//MY_LOG(INFO) << "QueueFamilyProperties : " << i << "\tflags:"
			//          << vk::to_string(queueFamilyProperties[i].queueFlags) ;
			if (phyDevice.getSurfaceSupportKHR(i, surface) &&
			    (queueFamilyProperties[i].queueFlags & flags)) {
				MY_LOG(INFO) << "default_queue_index :" << i << "\tgetSurfaceSupportKHR:true";
				return i;
			}
		}
		throw std::logic_error{"queueFamilyPropertiesFindFlags Error"};
	}

	using AAssetHander = std::unique_ptr<AAsset, std::function<void(AAsset *)>>;

	inline auto AAssetManagerFileOpen(AAssetManager *assetManager, const std::string &filePath) {
		return AAssetHander{
				AAssetManager_open(assetManager, filePath.c_str(),
				                   AASSET_MODE_STREAMING),
				[](AAsset *AAsset) {
					AAsset_close(AAsset);
				}
		};
	}

	struct RenderpassBeginHandle : public vk::CommandBuffer {
		RenderpassBeginHandle(vk::CommandBuffer commandBuffer,
		                      vk::RenderPassBeginInfo renderPassBeginInfo)
				: vk::CommandBuffer{commandBuffer} {
			beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		}

		~RenderpassBeginHandle() {
			endRenderPass();
		}

		RenderpassBeginHandle(const RenderpassBeginHandle &) = delete; // non construction-copyable
		RenderpassBeginHandle &operator=(const RenderpassBeginHandle &) = delete; // non copyable
	};

	struct CommandBufferBeginHandle : public vk::CommandBuffer {
		CommandBufferBeginHandle(
				vk::UniqueCommandBuffer &uniqueCommandBuffer,
				vk::CommandBufferUsageFlags commandBufferUsageFlags = vk::CommandBufferUsageFlagBits{},
				vk::CommandBufferInheritanceInfo *pCommandBufferInheritanceInfo = nullptr) :
				vk::CommandBuffer{uniqueCommandBuffer.get()} {
			begin(vk::CommandBufferBeginInfo{commandBufferUsageFlags,
			                                 pCommandBufferInheritanceInfo});
		}

		~CommandBufferBeginHandle() {
			end();
		}

		CommandBufferBeginHandle(
				const CommandBufferBeginHandle &) = delete; // non construction-copyable
		CommandBufferBeginHandle &
		operator=(const CommandBufferBeginHandle &) = delete; // non copyable
	};

	uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice, uint32_t memoryTypeBits,
	                             vk::MemoryPropertyFlags flags);


	using ImageViewMemory = std::tuple<vk::UniqueImage, vk::UniqueImageView, vk::UniqueDeviceMemory>;

	//using ImageViewSamplerMemory = std::tuple<vk::UniqueImage, vk::UniqueImageView, vk::UniqueSampler, vk::UniqueDeviceMemory>;

	using BufferMemory = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, size_t, std::vector<vk::DescriptorBufferInfo> >;

	using LocalBufferMemory = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory>;

	using StagingBufferMemory = LocalBufferMemory;


	template<typename T = std::vector<vk::DescriptorBufferInfo>>
	struct DescriptorsBuffer : public std::tuple<vk::UniqueBuffer, size_t, T> {
		auto &buffer() {
			return std::get<vk::UniqueBuffer>(*this);
		}

		auto &off() {
			return std::get<size_t>(*this);
		}

		auto &descriptors() {
			return std::get<T>(*this);
		}

		using std::tuple<vk::UniqueBuffer, size_t, T>::tuple;
	};

	template<typename T = std::vector<DescriptorsBuffer<>>>
	struct BuffersMemory : public std::tuple<vk::UniqueDeviceMemory, size_t, T> {
		auto &memory() {
			return std::get<vk::UniqueDeviceMemory>(*this);
		}

		auto &size() {
			return std::get<size_t>(*this);
		}

		auto &desAndBuffers() {
			return std::get<T>(*this);
		}

		using std::tuple<vk::UniqueDeviceMemory, size_t, T>::tuple;
	};

	class BufferMemoryPtr : public std::unique_ptr<void, std::function<void(void *)> > {
	public:
		using std::unique_ptr<void, std::function<void(void *)> >::unique_ptr;

		template<typename PodType>
		PodType &PodTypeOnMemory() {
			return *static_cast<PodType *>(get());
		}

	};

	template<typename PodType>
	struct BufferTypePtr : public std::unique_ptr<PodType[], std::function<void(PodType *)> > {
		using std::unique_ptr<PodType[], std::function<void(PodType *)> >::unique_ptr;
	};

	namespace helper {

		template<typename PodType, typename Tuple>
		auto mapTypeMemoryAndSize(vk::Device device, Tuple &tupleMemoryAndSize, size_t offset = 0) {
			auto devMemory = std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get();
			return BufferTypePtr<PodType>{
					static_cast<PodType *>(device.mapMemory(
							std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
							offset,
							std::get<size_t>(tupleMemoryAndSize),
							vk::MemoryMapFlagBits())),
					[device, devMemory](PodType *pVoid) {
						//FIXME call ~PodType in array
						device.unmapMemory(devMemory);
					}
			};
		}

		template<typename JobType>
		auto createCmdBuffers(vk::Device device,
		                      vk::RenderPass renderPass,
		                      JobType &job,
		                      std::vector<vk::UniqueFramebuffer> &framebuffers,
		                      vk::Extent2D extent2D,
		                      vk::CommandPool pool) {
			MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << framebuffers.size();
			std::vector commandBuffers = device.allocateCommandBuffersUnique(
					vk::CommandBufferAllocateInfo{
							pool,
							vk::CommandBufferLevel::ePrimary,
							framebuffers.size()
					}
			);

			uint32_t frameIndex = 0;
			for (auto &cmdBuffer : commandBuffers) {
				//cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
				{
					CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
					job.CmdBufferBegin(cmdBeginHandle, extent2D);
					{
						RenderpassBeginHandle cmdHandleRenderpassBegin{
								cmdBeginHandle,
								vk::RenderPassBeginInfo{
										renderPass,
										framebuffers[frameIndex].get(),
										vk::Rect2D{
												vk::Offset2D{},
												extent2D
										},
										job.clearValues.size(), job.clearValues.data()
								}
						};
						job.CmdBufferRenderpassBegin(cmdHandleRenderpassBegin, extent2D);
					}

				}
				++frameIndex;
			}
			return commandBuffers;
		}


		template<typename JobType>
		auto createCmdBuffersSub(vk::Device device,
		                         vk::RenderPass renderPass,
		                         JobType &job,
		                         std::vector<vk::UniqueFramebuffer> &framebuffers,
		                         vk::Extent2D extent2D,
		                         vk::CommandPool pool) {
			MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << framebuffers.size();
			std::vector commandBuffers = device.allocateCommandBuffersUnique(
					vk::CommandBufferAllocateInfo{
							pool,
							vk::CommandBufferLevel::eSecondary,
							framebuffers.size()
					}
			);

			uint32_t frameIndex = 0;
			for (auto &cmdBuffer : commandBuffers) {
				//cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
				{
					vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo{renderPass, 0,
					                                                              framebuffers[frameIndex].get()};
					CommandBufferBeginHandle cmdHandleRenderpassBegin{cmdBuffer,
					                                                  vk::CommandBufferUsageFlagBits::eRenderPassContinue,
					                                                  &commandBufferInheritanceInfo};
					job.CmdBufferRenderPassContinueBegin(cmdHandleRenderpassBegin, extent2D);
				}
				++frameIndex;
			}
			return commandBuffers;
		}

	}
}

#endif //STBOX_UTIL_H
