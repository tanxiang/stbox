//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOB_HH
#define STBOX_JOB_HH

#include "util.hh"

namespace tt {
	class Window;

	struct Job {
		//tt::Device &device;
		vk::UniqueDescriptorPool descriptorPoll;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;//todo vector UniqueDescriptorSetLayout
		std::vector<vk::UniqueDescriptorSet> descriptorSets;

		//vk::UniqueRenderPass renderPass;
		vk::UniquePipelineCache pipelineCache;
		vk::UniquePipelineLayout pipelineLayout;//todo vector
		vk::UniqueCommandPool commandPool;
		vk::UniquePipeline uniquePipeline;//todo vector
		std::vector<vk::UniqueCommandBuffer> cmdBuffers;

		std::function<void(RenderpassBeginHandle &,vk::Extent2D)> cmdbufferRenderpassBeginHandle;
		std::function<void(CommandBufferBeginHandle &,vk::Extent2D)> cmdbufferCommandBufferBeginHandle;

		auto clearCmdBuffer() {
			return cmdBuffers.clear();
		}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		//memory using
		std::vector<BufferViewMemory> BVMs;

		void writeBvm(uint32_t index, void *data, size_t writeSize, size_t offset = 0) {
			//todo check index size
			if (writeSize + offset > std::get<size_t>(BVMs[index]))
				throw std::logic_error{"write buffer overflow!"};
			auto pMemory = helper::mapMemoryAndSize(descriptorPoll.getOwner(), BVMs[index], offset);
			memcpy(pMemory.get(), data, writeSize);

		}

		std::vector<ImageViewMemory> IVMs;
		vk::UniqueSampler sampler;

		Job(vk::Device device, uint32_t queueIndex,
		    std::vector<vk::DescriptorPoolSize> &&descriptorPoolSizes,
		    std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings/*vk::RenderPassCreateInfo &&renderPassCreateInfo,*/
		) :
		//device{device},
				descriptorPoll{
						device.createDescriptorPoolUnique(
								vk::DescriptorPoolCreateInfo{
										vk::DescriptorPoolCreateFlags(), 3,
										descriptorPoolSizes.size(),
										descriptorPoolSizes.data()
								}
						)
				},/*todo maxset unset*/
				descriptorSetLayout{
						device.createDescriptorSetLayoutUnique(
								vk::DescriptorSetLayoutCreateInfo{
										vk::DescriptorSetLayoutCreateFlags(),
										descriptorSetLayoutBindings.size(),
										descriptorSetLayoutBindings.data()
								}
						)
				},
				descriptorSets{
						device.allocateDescriptorSetsUnique(
								vk::DescriptorSetAllocateInfo{descriptorPoll.get(), 1,
								                              descriptorSetLayout.operator->()
								}
						)
				},//todo multi descriptorSetLayout
				/* renderPass{device->createRenderPassUnique(renderPassCreateInfo)},*/
				pipelineCache{device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo{})},
				pipelineLayout{
						device.createPipelineLayoutUnique(
								vk::PipelineLayoutCreateInfo{
										vk::PipelineLayoutCreateFlags(), 1,
										descriptorSetLayout.operator->(), 0,
										nullptr
								}
						)
				},
				commandPool{device.createCommandPoolUnique(vk::CommandPoolCreateInfo{
						vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex})} {
		}
	};
}

#endif //STBOX_JOB_HH
