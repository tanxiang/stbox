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

	class Window;

	class JobBase;

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
	inline auto AAssetManagerFileOpen(AAssetManager* assetManager,const std::string &filePath){
		return AAssetHander {
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
				vk::UniqueCommandBuffer &uniqueCommandBuffer,vk::CommandBufferUsageFlags commandBufferUsageFlags = vk::CommandBufferUsageFlagBits {}) :
				vk::CommandBuffer{uniqueCommandBuffer.get()} {
			begin(vk::CommandBufferBeginInfo{commandBufferUsageFlags});
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

	using BufferMemory = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, size_t ,std::vector<vk::DescriptorBufferInfo> >;

	class BufferMemoryPtr : public std::unique_ptr<void, std::function<void(void *)> > {
	public:
		using std::unique_ptr<void, std::function<void(void *)> >::unique_ptr;
		template<typename PodType>
		PodType& PodTypeOnMemory(){
			return *static_cast<PodType *>(get());
		}

	};
	namespace helper {
		template<typename Tuple>
		auto mapMemoryAndSize(vk::Device device, Tuple &tupleMemoryAndSize, size_t offset = 0) {
			return BufferMemoryPtr{
					device.mapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
					                 offset,
					                 std::get<size_t>(tupleMemoryAndSize),
					                 vk::MemoryMapFlagBits()),
					[device, &tupleMemoryAndSize](void *pVoid) {
						device.unmapMemory(
								std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get());
					}
			};
		}
	}
}

#endif //STBOX_UTIL_H
