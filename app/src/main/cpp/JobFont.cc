//
// Created by ttand on 19-9-26.
//

#include "JobFont.hh"

namespace tt {
	struct fd_Rect {
		float min_x;
		float min_y;
		float max_x;
		float max_y;
	};
	struct fd_GlyphInstance {
		fd_Rect rect;
		uint32_t glyph_index;
		float sharpness;
	};


	JobFont JobFont::create(android_app *app, tt::Device &device) {
		return JobFont{
				device.createJobBase(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eStorageBuffer, 3
								}
						},1
				),
				device, app
		};
	}

	vk::UniqueRenderPass JobFont::createRenderpass(tt::Device &device) {
		std::array attachDescs{
				vk::AttachmentDescription{
						vk::AttachmentDescriptionFlags(),
						device.getRenderPassFormat(),
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eLoad,
						vk::AttachmentStoreOp::eStore,
						vk::AttachmentLoadOp::eDontCare,
						vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::ePresentSrcKHR,
						vk::ImageLayout::ePresentSrcKHR
				},
				vk::AttachmentDescription{
						vk::AttachmentDescriptionFlags(),
						device.getDepthFormat(),
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eLoad,
						vk::AttachmentStoreOp::eStore,
						vk::AttachmentLoadOp::eDontCare,
						vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eDepthStencilAttachmentOptimal
				}
		};
		std::array attachmentRefs{
				vk::AttachmentReference{
						0, vk::ImageLayout::eColorAttachmentOptimal
				}
		};
		vk::AttachmentReference depthAttacheRefs{
				1, vk::ImageLayout::eDepthStencilAttachmentOptimal
		};
		std::array subpassDescs{
				vk::SubpassDescription{
						vk::SubpassDescriptionFlags(),
						vk::PipelineBindPoint::eGraphics,
						0, nullptr,
						attachmentRefs.size(), attachmentRefs.data(),
						nullptr,
						&depthAttacheRefs,
				}
		};

		std::array subpassDeps{
				vk::SubpassDependency{
						VK_SUBPASS_EXTERNAL, 0,
						vk::PipelineStageFlagBits::eBottomOfPipe,
						vk::PipelineStageFlagBits::eColorAttachmentOutput,
						vk::AccessFlagBits::eMemoryRead,
						vk::AccessFlagBits::eColorAttachmentRead |
						vk::AccessFlagBits::eColorAttachmentWrite,
						vk::DependencyFlagBits::eByRegion
				},
				vk::SubpassDependency{
						0, VK_SUBPASS_EXTERNAL,
						vk::PipelineStageFlagBits::eColorAttachmentOutput,
						vk::PipelineStageFlagBits::eBottomOfPipe,
						vk::AccessFlagBits::eColorAttachmentRead |
						vk::AccessFlagBits::eColorAttachmentWrite,
						vk::AccessFlagBits::eMemoryRead,
						vk::DependencyFlagBits::eByRegion
				}
		};
		return device->createRenderPassUnique(
				vk::RenderPassCreateInfo{
						vk::RenderPassCreateFlags(), attachDescs.size(), attachDescs.data(),
						subpassDescs.size(), subpassDescs.data(),
						subpassDeps.size(), subpassDeps.data()
				}
		);
	}

	vk::UniquePipeline JobFont::createPipeline(Device &device, android_app *app,vk::PipelineLayout pipelineLayout) {
		auto vertShaderModule = device.loadShaderFromAssets("shaders/font.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/font.frag.spv", app);
		std::array pipelineShaderStageCreateInfos{
				vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eVertex,
						vertShaderModule.get(), "main"
				},
				vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eFragment,
						fargShaderModule.get(), "main"
				}
		};
		std::array vertexInputBindingDescriptions{
				vk::VertexInputBindingDescription{
						0, sizeof(fd_GlyphInstance),
						vk::VertexInputRate::eInstance
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32Uint, 16
				},//VK_FORMAT_R32G32_SFLOAT
				vk::VertexInputAttributeDescription{
						2, 0, vk::Format::eR32Sfloat, 20
				}
		};
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				vk::PipelineVertexInputStateCreateFlags(),
				vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};

		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,pipelineLayout,
		                                   pipelineCache.get(), device.renderPass.get());
	}


	void JobFont::buildCmdBuffer(tt::Window &swapchain) {
		cmdBuffers = helper::createCmdBuffers(ownerDevice(), renderPass.get(), *this,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(), commandPool.get());
	}

	void JobFont::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass cmdrenderPass) {
		cmdBuffers = helper::createCmdBuffers(ownerDevice(), cmdrenderPass, *this,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(), commandPool.get());
	}

	void JobFont::CmdBufferRenderpassBegin(RenderpassBeginHandle &handle, vk::Extent2D win) {

		handle.setViewport(
				0,
				std::array{
						vk::Viewport{
								0, 0,
								win.width,
								win.height,
								0.0f, 1.0f
						}
				}
		);
		std::array offsets{vk::DeviceSize{0}};
		handle.setScissor(0, std::array{vk::Rect2D{vk::Offset2D{}, win}});
		handle.bindPipeline(vk::PipelineBindPoint::eGraphics, graphPipeline.get());
		//std::array tmpDescriptorSets{descriptorSets[0].get()};
		handle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphPipeline.layout(), 0,
		                          graphPipeline.getDescriptorSet(), {});
		handle.bindVertexBuffers(0, std::get<vk::UniqueBuffer>(BAMs[2]).get(), offsets);
		handle.draw(4, 6, 0, 0);
	}

	void JobFont::CmdBufferBegin(CommandBufferBeginHandle &handle, vk::Extent2D) {
		handle.pipelineBarrier(
				vk::PipelineStageFlagBits::eVertexInput,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion,
				{},
				std::array{
						vk::BufferMemoryBarrier{
								vk::AccessFlagBits::eVertexAttributeRead,
								vk::AccessFlagBits::eTransferWrite,
								0, 0,//FIXME QueueFamilyIndex undefine
								std::get<vk::UniqueBuffer>(BAMs[2]).get(),
								0, 128 * sizeof(fd_GlyphInstance)
						}
				},
				{}
		);

		handle.copyBuffer(
				std::get<vk::UniqueBuffer>(BAMs[1]).get(),
				std::get<vk::UniqueBuffer>(BAMs[2]).get(),
				{vk::BufferCopy{0, 0, 128 * sizeof(fd_GlyphInstance)}}
		);

		handle.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eVertexInput,
				vk::DependencyFlagBits::eByRegion,
				{},
				std::array{
						vk::BufferMemoryBarrier{
								vk::AccessFlagBits::eTransferWrite,
								vk::AccessFlagBits::eVertexAttributeRead,
								0, 0,//FIXME QueueFamilyIndex undefine
								std::get<vk::UniqueBuffer>(BAMs[2]).get(),
								0, 128 * sizeof(fd_GlyphInstance)
						}
				},
				{}
		);

	}

	JobFont::JobFont(JobBase &&j, Device &device, android_app *app) :
			JobBase{std::move(j)},
			renderPass{createRenderpass(device)},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createPipeline(device, app, pipelineLayout);
					},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eVertex
							},
							vk::DescriptorSetLayoutBinding{
									1, vk::DescriptorType::eCombinedImageSampler,
									1, vk::ShaderStageFlagBits::eFragment
							}
					}
			}
			 {
		auto &fontBVM = BAMs.emplace_back(
				device.createBufferAndMemoryFromAssets(
						app, {"glyhps/glyphy_3072.bin", "glyhps/cell_21576.bin",
						      "glyhps/point_47440.bin"},
						vk::BufferUsageFlagBits::eStorageBuffer,
						vk::MemoryPropertyFlagBits::eDeviceLocal)
		);

		std::array descriptorWriteInfos{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1, vk::DescriptorType::eStorageBuffer,
						nullptr, &std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[0]
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 1, 0, 1, vk::DescriptorType::eStorageBuffer,
						nullptr, &std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[1]
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 2, 0, 1, vk::DescriptorType::eStorageBuffer,
						nullptr, &std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[2]
				}
		};
		device->updateDescriptorSets(descriptorWriteInfos, nullptr);
		{
			auto &inputBM = BAMs.emplace_back(
					device.createBufferAndMemory(
							sizeof(fd_GlyphInstance) * 128, vk::BufferUsageFlagBits::eVertexBuffer,
							vk::MemoryPropertyFlagBits::eHostVisible |
							vk::MemoryPropertyFlagBits::eHostCoherent
					)
			);

			auto inputPtr = helper::mapTypeMemoryAndSize<fd_GlyphInstance>(ownerDevice(), inputBM);

			std::string c{"CLXLOD"};
			for (int i = 0; i < c.size(); i++) {
				inputPtr[i].glyph_index = c[i] - 32;
				//MY_LOG(INFO) << " put : "<<inputPtr[i].glyph_index;
				inputPtr[i].rect = {i, -0.759358, -0.869379, -0.912940};
				inputPtr[i].sharpness = 0.8;
			}

		}
		auto &devcharBM = BAMs.emplace_back(
				device.createBufferAndMemory(
						sizeof(fd_GlyphInstance) * 128, vk::BufferUsageFlagBits::eVertexBuffer,
						vk::MemoryPropertyFlagBits::eDeviceLocal
				)
		);
	}
}
