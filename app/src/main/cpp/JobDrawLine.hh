//
// Created by ttand on 19-11-11.
//

#ifndef STBOX_JOBDRAWLINE_HH
#define STBOX_JOBDRAWLINE_HH

#include "util.hh"
//#include "Device.hh"
#include "PipelineResource.hh"
#include "thread.hh"
#include "JobBase.hh"

#include <android_native_app_glue.h>


namespace tt {
	struct JobDrawLine : public JobBase{
		vk::UniqueRenderPass renderPass;
		vk::UniqueRenderPass createRenderpass(tt::Device &);

		std::array<vk::ClearValue, 2> clearValues{
				vk::ClearColorValue{std::array<float, 4>{0.1f, 0.5f, 0.5f, 0.2f}},
				vk::ClearDepthStencilValue{1.0f, 0},
		};

		PipelineResource compPipeline;
		PipelineResource graphPipeline;


		BuffersMemory<> Bsm;
		BufferMemory outputMemory;
		vk::UniqueCommandBuffer cCommandBuffer;
		Thread worker;

		vk::UniquePipeline createGraphsPipeline(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);
		vk::UniquePipeline createComputePipeline(tt::Device &, android_app *app,vk::PipelineLayout pipelineLayout);

		static JobDrawLine create(android_app *app, tt::Device &device);

		static JobBase createBase(tt::Device &device);

		JobDrawLine(JobBase&& j,android_app *app,tt::Device &device);

		template <typename tupleType>
		JobDrawLine(tupleType args):JobDrawLine(std::move(std::get<JobBase>(args)),std::get<android_app*>(args),*std::get<tt::Device*>(args)){}



		//JobDrawLine(JobDrawLine&& j) = default;
	};
}

#endif //STBOX_JOBDRAWLINE_HH
