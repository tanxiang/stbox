//
// Created by ttand on 19-8-2.
//

#include "Job.hh"
#include "Window.hh"
namespace tt{

	vk::UniqueFence Window::submitCmdBuffer(Device &device,
	                                        std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers,
	                                        vk::Semaphore &imageAcquiredSemaphore,
	                                        vk::Semaphore &renderSemaphore) {
		auto currentBufferIndex = acquireNextImage(device, imageAcquiredSemaphore);

		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		std::array submitInfos{
				vk::SubmitInfo{
						1, &imageAcquiredSemaphore, &pipelineStageFlags,
						1, &drawcommandBuffers[currentBufferIndex.value].get(),
						1, &renderSemaphore
				}
		};
		auto renderFence = device->createFenceUnique(vk::FenceCreateInfo{});
		device.graphsQueue().submit(submitInfos, renderFence.get());
		auto presentRet = queuePresent(device.graphsQueue(), currentBufferIndex.value,
		                               renderSemaphore);
		MY_LOG(INFO) << "index:" << currentBufferIndex.value << "\tpresentRet:"
		             << vk::to_string(presentRet);
		return renderFence;
	}

	void Window::submitCmdBufferAndWait(Device &device,
	                                    std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers) {
		auto imageAcquiredSemaphore = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		auto renderSemaphore = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		auto renderFence = submitCmdBuffer(device, drawcommandBuffers, imageAcquiredSemaphore.get(),
		                                   renderSemaphore.get());
		device.waitFence(renderFence.get());
		//wait renderFence then free renderSemaphore imageAcquiredSemaphore
	}

	Window::Window(vk::UniqueSurfaceKHR &&sf, tt::Device &device, vk::Extent2D windowExtent)
			: surface{std::move(sf)}, swapchainExtent{windowExtent} {
		auto physicalDevice = device.phyDevice();
		auto surfaceCapabilitiesKHR = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
		//MY_LOG(INFO) << "AndroidGetWindowSize() : " << swapchainExtent.width << " x "
		//          << swapchainExtent.height ;
		if (surfaceCapabilitiesKHR.currentExtent.width == 0xFFFFFFFF) {
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			if (swapchainExtent.width < surfaceCapabilitiesKHR.minImageExtent.width) {
				swapchainExtent.width = surfaceCapabilitiesKHR.minImageExtent.width;
			} else if (swapchainExtent.width > surfaceCapabilitiesKHR.maxImageExtent.width) {
				swapchainExtent.width = surfaceCapabilitiesKHR.maxImageExtent.width;
			}
			if (swapchainExtent.height < surfaceCapabilitiesKHR.minImageExtent.height) {
				swapchainExtent.height = surfaceCapabilitiesKHR.minImageExtent.height;
			} else if (swapchainExtent.height > surfaceCapabilitiesKHR.maxImageExtent.height) {
				swapchainExtent.height = surfaceCapabilitiesKHR.maxImageExtent.height;
			}
		} else {
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfaceCapabilitiesKHR.currentExtent;
		}
		MY_LOG(INFO) << "swapchainExtent : " << swapchainExtent.width << " x "
		             << swapchainExtent.height;
		auto surfacePresentMods = physicalDevice.getSurfacePresentModesKHR(surface.get());

		//if(std::find(surfacePresentMods.begin(),surfacePresentMods.end(),vk::PresentModeKHR::eMailbox) != surfacePresentMods.end()){
		//    MY_LOG(INFO) << "surfacePresentMods have: eMailbox\n";
		//}
		for (auto &surfacePresentMod :surfacePresentMods) {
			MY_LOG(INFO) << "\t\tsurfacePresentMods had " << vk::to_string(surfacePresentMod);
		}
		uint32_t desiredNumberOfSwapchainImages = surfaceCapabilitiesKHR.minImageCount;
		//auto surfaceDefaultFormat = device.getSurfaceDefaultFormat(surface.get());
		vk::SurfaceTransformFlagBitsKHR preTransform{};
		if (surfaceCapabilitiesKHR.supportedTransforms &
		    vk::SurfaceTransformFlagBitsKHR::eIdentity) {
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		} else {
			preTransform = surfaceCapabilitiesKHR.currentTransform;
		}
		vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
				vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
				vk::CompositeAlphaFlagBitsKHR::eInherit
		};
		for (auto &compositeAlphaFlag:compositeAlphaFlags) {
			if (surfaceCapabilitiesKHR.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			}
		}
		assert(surfaceCapabilitiesKHR.minImageCount <= desiredNumberOfSwapchainImages &&
		       surfaceCapabilitiesKHR.maxImageCount >= desiredNumberOfSwapchainImages);
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{vk::SwapchainCreateFlagsKHR(),
		                                               surface.get(),
		                                               desiredNumberOfSwapchainImages,
		                                               device.getRenderPassFormat(),
		                                               vk::ColorSpaceKHR::eSrgbNonlinear,
		                                               swapchainExtent,
		                                               1,
		                                               vk::ImageUsageFlagBits::eColorAttachment |
		                                               vk::ImageUsageFlagBits::eTransferSrc,
		                                               vk::SharingMode::eExclusive,
		                                               0,//TODO to spt graphics and present queues from different queue families
		                                               nullptr,
		                                               preTransform,
		                                               compositeAlpha,
		                                               vk::PresentModeKHR::eMailbox,
		                                               true};

		//swap(device->createSwapchainKHRUnique(swapChainCreateInfo));
		swapchain = (device->createSwapchainKHRUnique(swapChainCreateInfo));

		auto swapChainImages = device->getSwapchainImagesKHR(swapchain.get());
		MY_LOG(INFO) << "swapChainImages size : " << swapChainImages.size();

		//imageViews.clear();
		for (auto &vkSwapChainImage : swapChainImages)
			imageViews.emplace_back(device->createImageViewUnique(vk::ImageViewCreateInfo{
					vk::ImageViewCreateFlags(),
					vkSwapChainImage,
					vk::ImageViewType::e2D,
					device.getRenderPassFormat(),
					vk::ComponentMapping{
							vk::ComponentSwizzle::eR,
							vk::ComponentSwizzle::eG,
							vk::ComponentSwizzle::eB,
							vk::ComponentSwizzle::eA},
					vk::ImageSubresourceRange{
							vk::ImageAspectFlagBits::eColor,
							0, 1, 0, 1}
			}));

		depth = device.createImageAndMemory(device.getDepthFormat(),
		                                    vk::Extent3D{swapchainExtent.width,
		                                                 swapchainExtent.height, 1});


		//frameBuffers.clear();
		for (auto &imageView : imageViews) {
			std::array attachments{imageView.get(), std::get<vk::UniqueImageView>(depth).get()};
			frameBuffers.emplace_back(
					device->createFramebufferUnique(
							vk::FramebufferCreateInfo{
									vk::FramebufferCreateFlags(),
									device.renderPass.get(),
									attachments.size(), attachments.data(),
									swapchainExtent.width, swapchainExtent.height,
									1
							}
					)
			);
		}

	}
}