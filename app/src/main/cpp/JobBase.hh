//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOBBASE_HH
#define STBOX_JOBBASE_HH

#include "util.hh"

namespace tt {

	struct JobBase {
		vk::UniqueDescriptorPool descriptorPoll;
		auto ownerDevice() {
			return descriptorPoll.getOwner();
		}

		vk::UniquePipelineCache pipelineCache;

		vk::UniqueCommandPool commandPool;

		JobBase(vk::Device device, uint32_t queueIndex,
		        vk::ArrayProxy<vk::DescriptorPoolSize> descriptorPoolSizes, size_t maxSet) :
				descriptorPoll{
						device.createDescriptorPoolUnique(
								vk::DescriptorPoolCreateInfo{
										vk::DescriptorPoolCreateFlags(),
										maxSet,
										descriptorPoolSizes.size(),
										descriptorPoolSizes.data()
								}
						)
				},
				commandPool{
						ownerDevice().createCommandPoolUnique(vk::CommandPoolCreateInfo{
								vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex}
						)
				} {}

	};
}

#endif //STBOX_JOBBASE_HH
