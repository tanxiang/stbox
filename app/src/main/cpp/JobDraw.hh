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
		vk::UniquePipeline createPipeline(tt::Device&,android_app* app);
		vk::UniquePipeline uniquePipeline;//todo vector
		std::vector<vk::UniqueCommandBuffer> cmdBuffers;

		glm::mat4 perspective;
		glm::vec3 camPos = glm::vec3(1, 3, 4);
		glm::vec3 camTo = glm::vec3(0, 0, 0);
		glm::vec3 camUp = glm::vec3(0, 1, 0);

		std::array<vk::ClearValue,2> clearValues{
				vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.2f, 0.2f}},
				vk::ClearDepthStencilValue{1.0f, 0},
		};

		void CmdBufferRenderpassBegin(RenderpassBeginHandle &, vk::Extent2D);
		std::function<void(CommandBufferBeginHandle &,
		                   vk::Extent2D)> CmdBufferBegin = [](
		                   		CommandBufferBeginHandle &,
		                   		vk::Extent2D) {};

		auto clearCmdBuffer() {
			return cmdBuffers.clear();
		}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void setPerspective(tt::Window &swapchain);

		void setPv(float dx = 0.0, float dy = 0.0);

		//memory using
		std::vector<BufferMemory> BAMs;
		std::vector<ImageViewMemory> IVMs;
		vk::UniqueSampler sampler;

		static JobDraw create(android_app *app, tt::Device &device);

		JobDraw(JobBase&& j,android_app *app,tt::Device &device);
	};
}

#endif //STBOX_JOBDRAW_HH
