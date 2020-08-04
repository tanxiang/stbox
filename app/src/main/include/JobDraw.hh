//
// Created by ttand on 19-9-25.
//
#pragma once

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

		void setPv();

		void setMVP(tt::Device &device){
			setPv();
		}
		//memory using
		std::vector<BufferMemory> BAMs;
		BuffersMemory<> Bsm;
		std::vector<ImageViewMemory> IVMs;
		vk::UniqueSampler sampler;

		static JobDraw create(android_app *app, tt::Device &device);

		JobDraw(JobBase&& j,android_app *app,tt::Device &device);
	};
}

