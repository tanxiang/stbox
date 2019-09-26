//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOBBASE_HH
#define STBOX_JOBBASE_HH

#include "util.hh"

namespace tt {

	struct JobBase {
		//tt::Device &device;
		vk::PhysicalDevice physicalDevice;
		vk::UniqueDescriptorPool descriptorPoll;
		vk::UniqueDescriptorSetLayout descriptorSetLayout;//todo vector UniqueDescriptorSetLayout
		std::vector<vk::UniqueDescriptorSet> descriptorSets;
		//vk::UniqueRenderPass renderPass;
		vk::UniquePipelineCache pipelineCache;
		vk::UniquePipelineLayout pipelineLayout;//todo vector
		vk::UniqueCommandPool commandPool;

		JobBase(tt::Device &device, uint32_t queueIndex,
		    std::vector<vk::DescriptorPoolSize> &&descriptorPoolSizes,
		    std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings);
	};
}

#endif //STBOX_JOBBASE_HH
