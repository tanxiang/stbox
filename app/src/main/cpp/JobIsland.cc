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

		auto vertShaderModule = device.loadShaderFromAssets("shaders/copy1.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/mesh.frag.spv", app);
		auto tescShaderModule = device.loadShaderFromAssets("shaders/passthrough.tesc.spv", app);
		auto teseShaderModule = device.loadShaderFromAssets("shaders/passthrough.tese.spv", app);

		std::array pipelineShaderStageCreateInfos{
				vk::PipelineShaderStageCreateInfo{
						{},
						vk::ShaderStageFlagBits::eVertex,
						vertShaderModule.get(),
						"main"
				},
				vk::PipelineShaderStageCreateInfo{
						{},
						vk::ShaderStageFlagBits::eTessellationControl,
						tescShaderModule.get(),
						"main"
				},
				vk::PipelineShaderStageCreateInfo{
						{},
						vk::ShaderStageFlagBits::eTessellationEvaluation,
						teseShaderModule.get(),
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
						0, sizeof(float) * 6,
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32B32Sfloat, 12
				}
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()
		};


		vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{{},3};
		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(),
		                                   device.renderPass.get(),
		                                   vk::PrimitiveTopology::ePatchList,
		                                   vk::PolygonMode::eLine,
		                                   pipelineTessellationStateCreateInfo);
	}

	JobIsland::JobIsland(android_app *app, tt::Device *device) :
			JobBase{
					device->createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									}
							},
							1
					)
			},
			graphPipeline{
					device->get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createGraphsPipeline(
								*device,
								app,
								pipelineLayout);
					},
					std::array{
							vk::PushConstantRange{vk::ShaderStageFlagBits::eFragment, 0,
							                      sizeof(float) * 16},
					},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eTessellationEvaluation
							}
					}
			},
			BAM{device->createBufferAndMemory(
					sizeof(glm::mat4) * 2,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent)} {

		///home/ttand/work/stbox/app/src/main/assets/models/torusknot.obj.ext
		///home/ttand/work/stbox/app/src/main/assets/models/untitled.obj.ext
		memoryWithPartsd = device->createBufferPartsdOnAssertDir(
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eTransferDst,
				app->activity->assetManager, "models/untitled.obj.ext",
				sizeof(glm::mat4) * 2);

		std::array descriptors{
				createDescriptorBufferInfoTuple(memoryWithPartsd,
				                                std::get<std::vector<uint32_t>>(
						                                memoryWithPartsd).size() - 1),
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[0]
				}
		};
		device->get().updateDescriptorSets(writeDes, nullptr);
		AAssetHander file{app->activity->assetManager, "models/untitled.obj.material.bin"};
		materials.resize(file.getLength() / 2);
		file.read(materials.data(), file.getLength());

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
		for (uint32_t partIndex = 0, matIndex = 0; partIndex < 99; partIndex += 3, ++matIndex) {
			cmdHandleRenderpassBegin.bindVertexBuffers(
					0, std::get<vk::UniqueBuffer>(memoryWithPartsd).get(),
					createDescriptorBufferInfoTuple(memoryWithPartsd, partIndex).offset
			);

			cmdHandleRenderpassBegin.bindIndexBuffer(
					std::get<vk::UniqueBuffer>(memoryWithPartsd).get(),
					createDescriptorBufferInfoTuple(memoryWithPartsd, partIndex + 1).offset,
					vk::IndexType::eUint16
			);
			std::array<float, 16> pushdata;
			auto material = materials.begin() + matIndex * 10;
			std::copy_n(material, 3, pushdata.begin());
			std::copy_n(material + 3, 3, pushdata.begin() + 4);
			std::copy_n(material + 6, 3, pushdata.begin() + 8);

			std::copy_n(std::array{0.5f, 0.5f, 0.5f,}.begin(), 3, pushdata.begin() + 12);

			cmdHandleRenderpassBegin.pushConstants(graphPipeline.layout(),
			                                       vk::ShaderStageFlagBits::eFragment, 0,
			                                       sizeof(float) * pushdata.size(),
			                                       pushdata.data()
			);
			cmdHandleRenderpassBegin.drawIndexedIndirect(
					std::get<vk::UniqueBuffer>(memoryWithPartsd).get(),
					createDescriptorBufferInfoTuple(memoryWithPartsd, partIndex + 2).offset,
					1, 0);
		}
		return;

	}

	void
	JobIsland::setMVP(tt::Device &device) {
		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAM);
			memory_ptr[1] = glm::mat4_cast(fRotate);
			memory_ptr[0] = perspective * lookat * memory_ptr[1];
		}
		auto buffer = std::get<vk::UniqueBuffer>(BAM).get();
		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(memoryWithPartsd).get(),
				device->getBufferMemoryRequirements(buffer).size,
				0,
				createDescriptorBufferInfoTuple(memoryWithPartsd,
				                                std::get<std::vector<uint32_t>>(
						                                memoryWithPartsd).size() - 1).offset);
	}
}