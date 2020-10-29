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
		gpuProgram<> graphPipeline;

		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		auto getGraphisCmdBuffer(uint32_t index){
			return cmdBuffers[index].get();
		}


		void CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassContinue,
		                                      vk::Extent2D win,uint32_t frameIndex);

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);//void setPv();

		void setMVP(tt::Device &device);
		//memory using
		BufferMemory BAM;
		vk::UniqueSampler sampler;
		BufferMemoryWithParts<2> bufferMemoryPart;

		//static JobDraw create(android_app *app, tt::Device &device);

		JobDraw(android_app *app,tt::Device *device);

		template <typename tupleType>
		JobDraw(tupleType args):JobDraw(std::get<android_app*>(args),std::get<tt::Device*>(args)){}
	};
}

