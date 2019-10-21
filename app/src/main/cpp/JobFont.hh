//
// Created by ttand on 19-9-26.
//

#ifndef STBOX_JOBFONT_HH
#define STBOX_JOBFONT_HH

#include "util.hh"
#include "Device.hh"
#include "JobBase.hh"

namespace tt {

	struct JobFont : public JobBase{
		vk::UniqueRenderPass createRenderpass(tt::Device&);
		vk::UniqueRenderPass renderPass;
		vk::UniquePipeline createPipeline(tt::Device&,android_app* app);
		vk::UniquePipeline uniquePipeline;//todo vector
		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		std::vector<BufferMemory> BAMs;

		void CmdBufferRenderpassBegin(RenderpassBeginHandle &, vk::Extent2D);

		void CmdBufferBegin(CommandBufferBeginHandle &, vk::Extent2D);


		void buildCmdBuffer(tt::Window &swapchain);

		static JobFont create(android_app *app, tt::Device &device);

		JobFont(JobBase&& j,Device& device,android_app* app):JobBase{std::move(j)},
		renderPass{createRenderpass(device)},
		uniquePipeline{createPipeline(device,app)}{}
	};
}

#endif //STBOX_JOBFONT_HH
