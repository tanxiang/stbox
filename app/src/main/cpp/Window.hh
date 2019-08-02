//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_WINDOW_HH
#define STBOX_WINDOW_HH

#include "util.hh"
#include "Device.hh"

namespace tt {
	class Window {
		vk::UniqueSurfaceKHR surface;
		vk::Extent2D swapchainExtent;
		vk::UniqueSwapchainKHR swapchain;
		std::vector<vk::UniqueImageView> imageViews;
		ImageViewMemory depth;
		std::vector<vk::UniqueFramebuffer> frameBuffers;

	public:
		//std::vector<vk::UniqueCommandBuffer> mianBuffers;

		//Swapchain(){}

		Window(vk::UniqueSurfaceKHR &&sf, tt::Device &device, vk::Extent2D windowExtent);

		vk::Extent2D getSwapchainExtent() {
			return swapchainExtent;
		}

		auto getFrameBufferNum() {
			return frameBuffers.size();
		}

		auto &getFrameBuffer() {
			return frameBuffers;
		}

		auto acquireNextImage(Device &device, vk::Semaphore imageAcquiredSemaphore) {
			return device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, imageAcquiredSemaphore,
			                                   vk::Fence{});
		}

		vk::UniqueFence submitCmdBuffer(Device &device,
		                                std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers,
		                                vk::Semaphore &imageAcquiredSemaphore,
		                                vk::Semaphore &renderSemaphore);

		void submitCmdBufferAndWait(Device &device,
		                            std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers);

		auto queuePresent(vk::Queue queue, uint32_t imageIndex,
		                  vk::Semaphore waitSemaphore = vk::Semaphore{}) {
			vk::PresentInfoKHR presentInfo{
					0, nullptr, 1, &swapchain.get(), &imageIndex,

			};
			if (waitSemaphore) {
				presentInfo.pWaitSemaphores = &waitSemaphore;
				presentInfo.waitSemaphoreCount = 1;
			}
			return queue.presentKHR(presentInfo);
		}

	};

}
#endif //STBOX_WINDOW_HH
