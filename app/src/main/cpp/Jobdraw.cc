//
// Created by ttand on 19-9-25.
//

#include "Jobdraw.hh"

namespace tt{

	std::vector<vk::UniqueCommandBuffer>
	createCmdBuffers(vk::Device device,
	                 vk::RenderPass renderPass,
	                 tt::Jobdraw &job,
	                 std::vector<vk::UniqueFramebuffer> &framebuffers, vk::Extent2D extent2D,
	                 vk::CommandPool pool,
	                 std::function<void(Jobdraw &, RenderpassBeginHandle &,
	                                    vk::Extent2D)> functionRenderpassBegin,
	                 std::function<void(Jobdraw &,CommandBufferBeginHandle &,
	                                    vk::Extent2D)> functionBegin) {
		MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << framebuffers.size();
		std::vector commandBuffers = device.allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo{
						pool,
						vk::CommandBufferLevel::ePrimary,
						framebuffers.size()
				}
		);

		std::array clearValues{
				vk::ClearValue{
						vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.2f, 0.2f}}},
				vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
		};
		uint32_t frameIndex = 0;
		for (auto &cmdBuffer : commandBuffers) {
			//cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			{
				CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
				functionBegin(job,cmdBeginHandle, extent2D);
				{
					RenderpassBeginHandle cmdHandleRenderpassBegin{
							cmdBeginHandle,
							vk::RenderPassBeginInfo{
									renderPass,
									framebuffers[frameIndex].get(),
									vk::Rect2D{
											vk::Offset2D{},
											extent2D
									},
									clearValues.size(), clearValues.data()
							}
					};
					functionRenderpassBegin(job,cmdHandleRenderpassBegin, extent2D);
				}

			}
			++frameIndex;
		}
		return commandBuffers;
	}

	void Jobdraw::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
//		MY_LOG(INFO)<<"jobaddr:"<<(void const *)this<<std::endl;

		cmdBuffers = createCmdBuffers(descriptorPoll.getOwner(), renderPass,
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

	void Jobdraw::setPerspective(tt::Window &swapchain) {
		perspective = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(swapchain.getSwapchainExtent().width) /
				static_cast<float>(swapchain.getSwapchainExtent().height),
				0.1f, 256.0f
		);
	}

	void Jobdraw::setPv(float dx, float dy) {
		camPos[0] -= dx * 0.1;
		camPos[1] -= dy * 0.1;
		bvmMemory(0).PodTypeOnMemory<glm::mat4>() = perspective * glm::lookAt(
				camPos,  // Camera is at (-5,3,-10), in World Space
				camTo,     // and looks at the origin
				camUp     // Head is up (set to 0,-1,0 to look upside-down)
		);
	}
};