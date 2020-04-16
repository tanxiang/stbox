//
// Created by ttand on 20-3-2.
//

#ifndef STBOX_JOBSKYBOX_HH
#define STBOX_JOBSKYBOX_HH

#include "util.hh"
//#include "Device.hh"
#include "PipelineResource.hh"
#include "thread.hh"
#include "JobBase.hh"
#include "Window.hh"

namespace tt {
	struct JobSkyBox: public JobBase {
		std::vector<vk::UniqueCommandBuffer> gcmdBuffers;
		PipelineResource graphPipeline;
		Thread worker;
		vk::UniquePipeline createGraphsPipeline(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);

		auto getGraphisCmdBuffer(uint32_t index){
			return gcmdBuffers[index].get();
		}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleBegin,
		                                      vk::Extent2D win,uint32_t frameIndex);

		void setMVP(tt::Device &device, vk::Buffer buffer);

		JobSkyBox(android_app *app,tt::Device &device);

		template <typename tupleType>
		JobSkyBox(tupleType args):JobSkyBox(std::get<android_app*>(args),*std::get<tt::Device*>(args)){}
		vk::UniqueSampler sampler;
		BufferImageMemoryWithParts<4> memoryWithParts;
		BufferMemory outputMemory;
	};
}

#endif //STBOX_JOBSKYBOX_HH
