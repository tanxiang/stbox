//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "vertexdata.hh"
#include "Device.hh"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

namespace tt{

	JobDraw JobDraw::create(android_app *app, tt::Device &device) {
		return JobDraw {
				device.createJob(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eUniformBuffer, 1
								},
								vk::DescriptorPoolSize{
										vk::DescriptorType::eCombinedImageSampler, 1
								},
						},
						{
								vk::DescriptorSetLayoutBinding{
										0, vk::DescriptorType::eUniformBuffer,
										1, vk::ShaderStageFlagBits::eVertex
								},
								vk::DescriptorSetLayoutBinding{
										1, vk::DescriptorType::eCombinedImageSampler,
										1, vk::ShaderStageFlagBits::eFragment
								}
						}
				),
				app,
				device
		};
	}

	vk::UniquePipeline JobDraw::createPipeline(Device& device,android_app* app) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);
		std::array shaderStageCreateInfos
		{
			vk::PipelineShaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				vertShaderModule.get(),
				"main"
			},
			vk::PipelineShaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				fargShaderModule.get(),
				"main"
			}
		};


		std::array vertexInputBindingDescriptions{
				vk::VertexInputBindingDescription{
						0, sizeof(VertexUV),
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32Sfloat, 16
				}//VK_FORMAT_R32G32_SFLOAT
		};
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				vk::PipelineVertexInputStateCreateFlags(),
				vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
				vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList
		};
		//vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};

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
				shaderStageCreateInfos.size(), shaderStageCreateInfos.data(),
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
				device.renderPass.get()
		};
		return device->createGraphicsPipelineUnique(pipelineCache.get(), pipelineCreateInfo);
	}
	void JobDraw::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
//		MY_LOG(INFO)<<"jobaddr:"<<(void const *)this<<std::endl;

		cmdBuffers = helper::createCmdBuffers(descriptorPoll.getOwner(), renderPass,
		                                      *this,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(),
		                                      commandPool.get());
		setPerspective(swapchain);
		//cmdBuffers = device.createCmdBuffers(swapchain, *commandPool, cmdbufferRenderpassBeginHandle,
		//                                     cmdbufferCommandBufferBeginHandle);
	}

	void JobDraw::setPerspective(tt::Window &swapchain) {
		perspective = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(swapchain.getSwapchainExtent().width) /
				static_cast<float>(swapchain.getSwapchainExtent().height),
				0.1f, 256.0f
		);
	}

	void JobDraw::setPv(float dx, float dy) {
		camPos[0] -= dx * 0.1;
		camPos[1] -= dy * 0.1;
		bvmMemory(0).PodTypeOnMemory<glm::mat4>() = perspective * glm::lookAt(
				camPos,  // Camera is at (-5,3,-10), in World Space
				camTo,     // and looks at the origin
				camUp     // Head is up (set to 0,-1,0 to look upside-down)
		);
	}

	void JobDraw::CmdBufferRenderpassBegin(RenderpassBeginHandle &cmdHandleRenderpassBegin,
			vk::Extent2D win) {
		std::array viewports{
			vk::Viewport{
					0, 0,
					win.width,
					win.height,
					0.0f, 1.0f
			}
		};
		cmdHandleRenderpassBegin.setViewport(0, viewports);
		std::array scissors{
			vk::Rect2D{vk::Offset2D{}, win}
		};
		cmdHandleRenderpassBegin.setScissor(0, scissors);

		cmdHandleRenderpassBegin.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			uniquePipeline.get());
		std::array tmpDescriptorSets{descriptorSets[0].get()};
		cmdHandleRenderpassBegin.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout.get(), 0,
			tmpDescriptorSets,
			{});
		vk::DeviceSize offsets[1] = {0};
		cmdHandleRenderpassBegin.bindVertexBuffers(0, 1,
				&std::get<vk::UniqueBuffer>(BAMs[1]).get(),
				offsets);
		cmdHandleRenderpassBegin.bindIndexBuffer(
				std::get<vk::UniqueBuffer>(BAMs[2]).get(),
				0, vk::IndexType::eUint32);
		cmdHandleRenderpassBegin.drawIndexed(6, 1, 0, 0, 0);
	}

JobDraw::JobDraw(JobBase &&j,android_app *app,tt::Device &device) :JobBase{std::move(j)}{
		BAMs.emplace_back(
				device.createBufferAndMemory(
						sizeof(glm::mat4),
						vk::BufferUsageFlagBits::eUniformBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		std::vector<VertexUV> vertices{
				{{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
				{{-1.0f, 1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
				{{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		};
		BAMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						vertices, vk::BufferUsageFlagBits::eVertexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		BAMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						std::vector<uint32_t>{0, 1, 2, 2, 3, 0},
						vk::BufferUsageFlagBits::eIndexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		{
			auto fileContent = loadDataFromAssets("textures/ic_launcher-web.ktx", app);
			//gli::texture2d tex2d;
			auto tex2d = gli::texture2d{gli::load_ktx(fileContent.data(), fileContent.size())};
			sampler = device.createSampler(tex2d.levels());
			IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));
		}


		uniquePipeline = createPipeline(device,app);

		auto descriptorBufferInfo = device.getDescriptorBufferInfo(BAMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(IVMs[0], sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						descriptorSets[0].get(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						descriptorSets[0].get(), 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);
//		MY_LOG(INFO)<<"jobaddr:"<<job<<std::endl;


	}
};