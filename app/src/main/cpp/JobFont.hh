//
// Created by ttand on 19-9-26.
//

#ifndef STBOX_JOBFONT_HH
#define STBOX_JOBFONT_HH

#include "util.hh"
#include "Device.hh"
#include "JobBase.hh"
#include "PipelineResource.hh"

namespace tt {
	struct JobFont : public JobBase {
		vk::UniqueRenderPass createRenderpass(tt::Device &);

		vk::UniqueRenderPass renderPass;
		std::array<vk::ClearValue, 2> clearValues{
				vk::ClearColorValue{std::array<float, 4>{0.1f, 0.5f, 0.5f, 0.2f}},
				vk::ClearDepthStencilValue{1.0f, 0},
		};

		vk::UniquePipeline createPipeline(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);

		PipelineResource graphPipeline;
		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		std::vector<BufferMemory> BAMs;

		void CmdBufferRenderpassBegin(RenderpassBeginHandle &, vk::Extent2D);

		void CmdBufferBegin(CommandBufferBeginHandle &, vk::Extent2D);

		void buildCmdBuffer(tt::Window &swapchain);

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass cmdrenderPass);

		static JobFont create(android_app *app, tt::Device &device);

		JobFont(JobBase &&j, Device &device, android_app *app);
	};
}

#endif //STBOX_JOBFONT_HH
