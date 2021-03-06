//
// Created by ttand on 19-11-11.
//
#pragma once

#include "util.hh"
//#include "Device.hh"
#include "PipelineResource.hh"
#include "thread.hh"
#include "JobBase.hh"
#include "Window.hh"


namespace tt {


	struct JobAabb: public JobBase{
		vk::UniqueRenderPass renderPass;
		//PipelineResource compPipeline;
		gpuProgram<2,2,2> compMprPipeline;
		gpuProgram<3> graphPipeline;

		BufferMemoryWithParts<13> bufferMemoryPart;
		//BufferMemory outputMemory;
		Thread worker;
		std::vector<vk::UniqueCommandBuffer> gcmdBuffers;
		std::vector<vk::UniqueCommandBuffer> cCmdbuffers;
		auto getGraphisCmdBuffer(uint32_t index){
			return gcmdBuffers[index].get();
		}
		auto getComputerCmdBuffer(){
			return *cCmdbuffers[0];
		}
		//vk::UniquePipeline createGraphsPipeline(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);
		//vk::UniquePipeline createGraphsPipelineCube(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);
		void createGraphsPipelines(std::array<vk::UniquePipeline,3>& pipelines,tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);

		void createComputePipeline(std::array<vk::UniquePipeline,2>& pipelines,tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);

		JobAabb(android_app *app,tt::Device &device);

		template <typename tupleType>
		JobAabb(tupleType args):JobAabb(std::get<android_app*>(args),*std::get<tt::Device*>(args)){}

		void buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass);

		void setMVP(tt::Device &device);

		void CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleBegin,
		                                       vk::Extent2D win,uint32_t frameIndex);

		BufferMemory BAM;

	};
}

