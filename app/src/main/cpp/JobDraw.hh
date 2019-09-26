//
// Created by ttand on 19-9-25.
//

#ifndef STBOX_JOBDRAW_HH
#define STBOX_JOBDRAW_HH

#include "util.hh"
#include "Device.hh"
#include "JobBase.hh"

namespace tt {
	struct JobDraw : public JobBase{

		vk::UniquePipeline uniquePipeline;//todo vector

		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		glm::mat4 perspective;

		glm::vec3 camPos = glm::vec3(1, 3, 4);
		glm::vec3 camTo = glm::vec3(0, 0, 0);
		glm::vec3 camUp = glm::vec3(0, 1, 0);

		std::function<void(JobDraw &, RenderpassBeginHandle &,
		                   vk::Extent2D)> cmdbufferRenderpassBeginHandle;
		std::function<void(JobDraw &, CommandBufferBeginHandle &,
		                   vk::Extent2D)> cmdbufferCommandBufferBeginHandle = [](JobDraw &,
		                                                                         CommandBufferBeginHandle &,
		                                                                         vk::Extent2D) {};

		auto clearCmdBuffer() {
			return cmdBuffers.clear();
		}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void setPerspective(tt::Window &swapchain);

		void setPv(float dx = 0.0, float dy = 0.0);

		//memory using
		std::vector<BufferMemory> BVMs;

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


		JobDraw(JobBase&& j):JobBase{std::move(j)}{}
	};
}

#endif //STBOX_JOBDRAW_HH
