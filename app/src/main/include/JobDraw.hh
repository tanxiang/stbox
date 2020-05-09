//
// Created by ttand on 19-9-25.
//

#ifndef STBOX_JOBDRAW_HH
#define STBOX_JOBDRAW_HH

#include "util.hh"
//#include "Device.hh"
#include "JobBase.hh"
#include "PipelineResource.hh"
#include "Window.hh"

namespace tt {
	struct JobDraw : public JobBase{
		vk::UniquePipeline createPipeline(tt::Device&,android_app* app,vk::PipelineLayout pipelineLayout);
		PipelineResource graphPipeline;

		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		auto getGraphisCmdBuffer(uint32_t index){
			return cmdBuffers[index].get();
		}


		void CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassContinue,
		                                      vk::Extent2D win,uint32_t frameIndex);

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void setPv(float dx = 0.0, float dy = 0.0);

		//memory using
		std::vector<BufferMemory> BAMs;
		BuffersMemory<> Bsm;
		std::vector<ImageViewMemory> IVMs;
		vk::UniqueSampler sampler;

		static JobDraw create(android_app *app, tt::Device &device);

		JobDraw(JobBase&& j,android_app *app,tt::Device &device);
	};
}

#endif //STBOX_JOBDRAW_HH
