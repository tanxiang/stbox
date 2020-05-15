//
// Created by ttand on 20-3-2.
//

#include "JobIsland.hh"
#include "Device.hh"
//#include <gli/gli.hpp>
//#include "model.hh"
//#include "ktx2.hh"

namespace tt {

	vk::UniquePipeline JobIsland::createGraphsPipeline(tt::Device &device, android_app *app,
	                                                   vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mesh.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/mesh.frag.spv", app);

		std::array pipelineShaderStageCreateInfos{
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
						0, sizeof(float) * 3,
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32Sfloat, 0
				}
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()
		};

		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(),
		                                   device.renderPass.get(),
		                                   vk::PrimitiveTopology::eTriangleStrip);
	}

	JobIsland::JobIsland(android_app *app, tt::Device &device) :
			JobBase{
					device.createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									}
							},
							1
					)
			},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createGraphsPipeline(
								device,
								app,
								pipelineLayout);
					},{},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eVertex
							}
					}
			} {

		memoryWithParts = device.createBufferPartsdOnAssertDir(
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eTransferDst,
				app->activity->assetManager,"models/IslaDEf.fbx.ext",
				sizeof(glm::mat4));
		return ;
		createDescriptorBufferInfoTuple(memoryWithParts, 1);
		std::array descriptors{
				createDescriptorBufferInfoTuple(memoryWithParts, 2),
				createDescriptorBufferInfoTuple(memoryWithParts, 3)
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[1]
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);
	}

	void JobIsland::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {

		gcmdBuffers = helper::createCmdBuffersSub(ownerDevice(), renderPass,
		                                          *this,
		                                          swapchain.getFrameBuffer(),
		                                          swapchain.getSwapchainExtent(),
		                                          commandPool.get());
	}

	void
	JobIsland::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassBegin,
	                                            vk::Extent2D win, uint32_t frameIndex) {
		return;
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
				0, std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 0).offset
		);

		cmdHandleRenderpassBegin.bindIndexBuffer(
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 1).offset,
				vk::IndexType::eUint16
		);

		cmdHandleRenderpassBegin.drawIndexedIndirect(
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 2).offset,
				1,0);
	}

	void
	JobIsland::setMVP(tt::Device &device, vk::Buffer buffer,vk::DeviceMemory deviceMemory) {
		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				device->getBufferMemoryRequirements(buffer).size,
				0,
				createDescriptorBufferInfoTuple(memoryWithParts, 3).offset);
	}
}