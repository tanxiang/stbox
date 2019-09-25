//
// Created by ttand on 19-8-2.
//

#include "Job.hh"
#include "Device.hh"
namespace tt {

	Job::Job(tt::Device
	         &device,
	         uint32_t queueIndex,
	         std::vector<vk::DescriptorPoolSize>
	         &&descriptorPoolSizes,
	         std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings) :
			physicalDevice {device.phyDevice()},
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
					device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}
					)},
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
					)} {
//auto phycompressFmts = device.phyDevice().getFormatProperties(vk::Format::eAstc4x4UnormBlock);
//MY_LOG(INFO)<<"vk::Format::eAstc4x4UnormBlock"<<vk::to_string(phycompressFmts.optimalTilingFeatures);
	}


void Job::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
//		MY_LOG(INFO)<<"jobaddr:"<<(void const *)this<<std::endl;

		cmdBuffers = helper::createCmdBuffers(descriptorPoll.getOwner(), renderPass,
		                                      *this,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(),
		                                      commandPool.get(),
		                                      cmdbufferRenderpassBeginHandle,
		                                      cmdbufferCommandBufferBeginHandle);
		setPerspective(swapchain);
		//cmdBuffers = device.createCmdBuffers(swapchain, *commandPool, cmdbufferRenderpassBeginHandle,
		//                                     cmdbufferCommandBufferBeginHandle);
	}

	void Job::setPerspective(tt::Window &swapchain) {
		perspective = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(swapchain.getSwapchainExtent().width) /
				static_cast<float>(swapchain.getSwapchainExtent().height),
				0.1f, 256.0f
		);
	}

	void Job::setPv(float dx, float dy) {
		camPos[0] -= dx * 0.1;
		camPos[1] -= dy * 0.1;
		bvmMemory(0).PodTypeOnMemory<glm::mat4>() = perspective * glm::lookAt(
				camPos,  // Camera is at (-5,3,-10), in World Space
				camTo,     // and looks at the origin
				camUp     // Head is up (set to 0,-1,0 to look upside-down)
		);
	}
}