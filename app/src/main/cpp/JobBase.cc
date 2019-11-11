//
// Created by ttand on 19-8-2.
//

#include "JobBase.hh"
#include "Device.hh"

namespace tt {

	JobBase::JobBase(tt::Device
	                 &device,
	                 uint32_t queueIndex,
	                 std::vector<vk::DescriptorPoolSize>
	                 &&descriptorPoolSizes,
	                 std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings) :
			physicalDevice{device.phyDevice()},
			descriptorPoll{
					device->createDescriptorPoolUnique(
							vk::DescriptorPoolCreateInfo{
									vk::DescriptorPoolCreateFlags(), 3,
									descriptorPoolSizes.size(),
									descriptorPoolSizes.data()
							}
					)
			},/*todo maxset unset*/
			descriptorSetLayout{
					device->createDescriptorSetLayoutUnique(
							vk::DescriptorSetLayoutCreateInfo{
									vk::DescriptorSetLayoutCreateFlags(),
									descriptorSetLayoutBindings.size(),
									descriptorSetLayoutBindings.data()
							}
					)
			},
			descriptorSets{
					device->allocateDescriptorSetsUnique(
							vk::DescriptorSetAllocateInfo{descriptorPoll.get(), 1,
							                              descriptorSetLayout.operator->()
							}
					)
			},//todo multi descriptorSetLayout
/* renderPass{device->createRenderPassUnique(renderPassCreateInfo)},*/
			pipelineCache{
					device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{})
			},
			pipelineLayout{
					device->createPipelineLayoutUnique(
							vk::PipelineLayoutCreateInfo{
									vk::PipelineLayoutCreateFlags(), 1,
									descriptorSetLayout.operator->(), 0,
									nullptr
							}
					)
			},
			commandPool{
					device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
							vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex}
					)} {}

}