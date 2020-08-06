//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "Device.hh"

struct Vertex {
	std::array<float, 4> pos;  // Position data
	std::array<float, 4> color; // Color
};

namespace tt {
	vk::UniquePipeline
	JobDraw::createPipeline(Device &device, android_app *app, vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvpc.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);
		std::array pipelineShaderStageCreateInfos
				{
						vk::PipelineShaderStageCreateInfo{
								{},
								vk::ShaderStageFlagBits::eVertex,
								vertShaderModule.get(),
								"main"
						},
						vk::PipelineShaderStageCreateInfo{
								{},
								vk::ShaderStageFlagBits::eFragment,
								fargShaderModule.get(),
								"main"
						}
				};


		std::array vertexInputBindingDescriptions{
				vk::VertexInputBindingDescription{
						0, sizeof(Vertex),
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32B32A32Sfloat, 16
				}//VK_FORMAT_R32G32_SFLOAT
		};
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};
		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(), device.renderPass.get(),
		                                   vk::PrimitiveTopology::eTriangleFan);

	}

	void JobDraw::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
		cmdBuffers = helper::createCmdBuffersSub(descriptorPool.getOwner(), renderPass,
		                                         *this,
		                                         swapchain.getFrameBuffer(),
		                                         swapchain.getSwapchainExtent(),
		                                         commandPool.get());

		//setPerspective(swapchain);
	}

	JobDraw::JobDraw(android_app *app, tt::Device &device) :
			JobBase{
					device.createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									}
									//,vk::DescriptorPoolSize{
									//		vk::DescriptorType::eCombinedImageSampler, 1
									//},
							}, 1
					)
			},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createPipeline(device, app, pipelineLayout);
					},
					{},//PushConst
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eVertex
							}
					}
			},
			BAM{device.createBufferAndMemory(
					sizeof(glm::mat4) * 2,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent)} {


		std::array Vertex_t{
				Vertex{{0.0, 0.7, 0.5, 1.0},
				       {1.0, 0.0, 0.0, 1.0}},
				Vertex{{0.7, -0.5, 0.5, 1.0},
				       {1.0, 1.0,  0.0, 1.0}},
				Vertex{{-0.7, -0.5, 0.5, 1.0},
				       {1.0,  1.0,  1.0, 1.0}},
				Vertex{{0.0, 0.0, 0.0, 1.0},
				       {0.0, 0.0,  0.0, 1.0}}};

		bufferMemoryPart = device.createBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eIndirectBuffer,
				Vertex_t,
				sizeof(glm::mat4));

		std::array descriptors{
				createDescriptorBufferInfoTuple(bufferMemoryPart, 1)
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[0]
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);
	}

	void JobDraw::setMVP(tt::Device &device) {
		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAM);
			memory_ptr[0] = perspective * lookat * glm::mat4_cast(fRotate);
		}
		auto buffer = std::get<vk::UniqueBuffer>(BAM).get();

		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				sizeof(glm::mat4),
				0,
				createDescriptorBufferInfoTuple(bufferMemoryPart, 1).offset);

	}


	void
	JobDraw::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassBegin,
	                                          vk::Extent2D win, uint32_t frameIndex) {
		cmdHandleRenderpassBegin.setViewport(
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
		cmdHandleRenderpassBegin.setScissor(0, std::array{vk::Rect2D{{}, win}});
		cmdHandleRenderpassBegin.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.get());

		cmdHandleRenderpassBegin.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.layout(), 0,
				graphPipeline.getDescriptorSets(),
				{}
		);

		cmdHandleRenderpassBegin.bindVertexBuffers(
				0, {std::get<vk::UniqueBuffer>(bufferMemoryPart).get()},
				{createDescriptorBufferInfoTuple(bufferMemoryPart, 0).offset}
		);

		cmdHandleRenderpassBegin.draw(4, 1, 0, 0);
	}
};