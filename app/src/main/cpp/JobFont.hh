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
		vk::UniqueRenderPass renderPass;
		vk::UniquePipeline uniquePipeline;//todo vector
		std::vector<vk::UniqueCommandBuffer> cmdBuffers;
		std::vector<BufferMemory> BVMs;
		static JobFont create(android_app *app, tt::Device &device);

		JobFont(JobBase&& j):JobBase{std::move(j)}{}
	};
}

#endif //STBOX_JOBFONT_HH
