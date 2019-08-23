//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOB_HH
#define STBOX_JOB_HH

#include "util.hh"

namespace tt {

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
		glm::mat4 perspective;

		glm::vec3 camPos = glm::vec3(8, 3, 5);
		glm::vec3 camTo = glm::vec3(0, 0, 0);
		glm::vec3 camUp = glm::vec3(0, 1, 0);

		std::function<void(RenderpassBeginHandle &, vk::Extent2D)> cmdbufferRenderpassBeginHandle;
		std::function<void(CommandBufferBeginHandle &,
		                   vk::Extent2D)> cmdbufferCommandBufferBeginHandle;

		auto clearCmdBuffer() {
			return cmdBuffers.clear();
		}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void setPerspective(tt::Window &swapchain);

		void setPv(float dx=0.0, float dy=0.0);

		//memory using
		std::vector<BufferViewMemory> BVMs;

		void writeBvm(uint32_t index, void *data, size_t writeSize, size_t offset = 0) {
			//todo check index size
			if (writeSize + offset > std::get<size_t>(BVMs[index]))
				throw std::logic_error{"write buffer overflow!"};
			auto pMemory = helper::mapMemoryAndSize(descriptorPoll.getOwner(), BVMs[index], offset);
			memcpy(pMemory.get(), data, writeSize);
		}

		auto bvmMemory(uint32_t index, size_t offset = 0) {
			return helper::mapMemoryAndSize(descriptorPoll.getOwner(), BVMs[index], offset);
		}

		std::vector<ImageViewMemory> IVMs;
		vk::UniqueSampler sampler;

		Job(vk::Device device, uint32_t queueIndex,
		    std::vector<vk::DescriptorPoolSize> &&descriptorPoolSizes,
		    std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings) :
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
