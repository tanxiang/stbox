//
// Created by ttand on 19-9-26.
//

#include "JobFont.hh"

namespace tt{
	struct fd_Rect
	{
		float min_x;
		float min_y;
		float max_x;
		float max_y;
	} ;
	struct fd_GlyphInstance
	{
		fd_Rect rect;
		uint32_t glyph_index;
		float sharpness;
	} ;

	JobFont JobFont::create(android_app *app, tt::Device &device) {
		JobFont job{
				device.createJob(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eStorageBuffer, 3
								}
						},
						{
								vk::DescriptorSetLayoutBinding{
										0, vk::DescriptorType::eStorageBuffer,
										1, vk::ShaderStageFlagBits::eVertex
								},
								{
										1, vk::DescriptorType::eStorageBuffer,
										1, vk::ShaderStageFlagBits::eFragment
								},
								{
										2, vk::DescriptorType::eStorageBuffer,
										1, vk::ShaderStageFlagBits::eFragment
								}
						}
				),
				device,app
		};
		auto& fontBVM = job.BVMs.emplace_back(
				device.createBufferAndMemoryFromAssets(
						app, {"glyhps/glyphy_3072.bin","glyhps/cell_21576.bin","glyhps/point_47440.bin"},
						vk::BufferUsageFlagBits::eStorageBuffer,
						vk::MemoryPropertyFlagBits::eDeviceLocal));

		std::array descriptorWriteInfos{
			vk::WriteDescriptorSet{
				job.descriptorSets[0].get(),0,0,1,vk::DescriptorType::eStorageBuffer,
				nullptr,&std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[0]
			},
			vk::WriteDescriptorSet{
				job.descriptorSets[0].get(),1,0,1,vk::DescriptorType::eStorageBuffer,
				nullptr,&std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[1]
			},
			vk::WriteDescriptorSet{
				job.descriptorSets[0].get(),2,0,1,vk::DescriptorType::eStorageBuffer,
				nullptr,&std::get<std::vector<vk::DescriptorBufferInfo>>(fontBVM)[2]
			}
		};
		device.get().updateDescriptorSets(descriptorWriteInfos, nullptr);

		return job;
	}

	vk::UniqueRenderPass JobFont::createRenderpass(tt::Device& device) {
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

	vk::UniquePipeline JobFont::createPipeline(Device& device,android_app* app) {
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
						vk::VertexInputRate::eVertex
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

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
				vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip
		};

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
				vk::PipelineViewportStateCreateFlags(),
				1, nullptr, 1, nullptr
		};
		std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{
				vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data()};


		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
				vk::PipelineRasterizationStateCreateFlags(),
				0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
				vk::FrontFace::eClockwise, 0,
				0, 0, 0, 1.0f
		};
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
				vk::PipelineDepthStencilStateCreateFlags(),
				true, true,
				vk::CompareOp::eLessOrEqual,
				false, false,
				vk::StencilOpState{
						vk::StencilOp::eKeep, vk::StencilOp::eKeep,
						vk::StencilOp::eKeep, vk::CompareOp::eNever
				},
				vk::StencilOpState{
						vk::StencilOp::eKeep, vk::StencilOp::eKeep,
						vk::StencilOp::eKeep, vk::CompareOp::eAlways
				},
				0, 0
		};
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
		pipelineColorBlendAttachmentState.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
				vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eClear, 1,
				&pipelineColorBlendAttachmentState
		};
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
				vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1};

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
				vk::PipelineCreateFlags(),
				pipelineShaderStageCreateInfos.size(), pipelineShaderStageCreateInfos.data(),
				&pipelineVertexInputStateCreateInfo,
				&pipelineInputAssemblyStateCreateInfo,
				nullptr,
				&pipelineViewportStateCreateInfo,
				&pipelineRasterizationStateCreateInfo,
				&pipelineMultisampleStateCreateInfo,
				&pipelineDepthStencilStateCreateInfo,
				&pipelineColorBlendStateCreateInfo,
				&pipelineDynamicStateCreateInfo,
				pipelineLayout.get(),
				renderPass.get()
		};
		//return vk::UniquePipeline{};
		return device->createGraphicsPipelineUnique(pipelineCache.get(),pipelineCreateInfo);
	}

}
