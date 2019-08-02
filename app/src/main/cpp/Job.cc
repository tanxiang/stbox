//
// Created by ttand on 19-8-2.
//

#include "Job.hh"
#include "Window.hh"

namespace tt{

	void Job::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
		cmdBuffers = helper::createCmdBuffers(descriptorPoll.getOwner(), renderPass,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(),
		                                      commandPool.get(),
		                                      cmdbufferRenderpassBeginHandle,
		                                      cmdbufferCommandBufferBeginHandle);
		//cmdBuffers = device.createCmdBuffers(swapchain, *commandPool, cmdbufferRenderpassBeginHandle,
		//                                     cmdbufferCommandBufferBeginHandle);
	}
}