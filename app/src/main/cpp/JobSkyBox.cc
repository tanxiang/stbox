//
// Created by ttand on 20-3-2.
//

#include "JobSkyBox.hh"
#include "Device.hh"
#include <gli/gli.hpp>
//#include "model.hh"

namespace tt {

	vk::UniquePipeline JobSkyBox::createGraphsPipeline(tt::Device &device, android_app *app,
	                                                   vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/skybox.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/skybox.frag.spv", app);

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
						0, sizeof(float) * 8,
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32B32Sfloat, sizeof(float) * 3
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

	JobSkyBox::JobSkyBox(android_app *app, tt::Device &device) :
			JobBase{
					device.createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									},
									vk::DescriptorPoolSize{
											vk::DescriptorType::eCombinedImageSampler, 1
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
			} {

		auto fileContent = loadDataFromAssets("textures/cubemap_yokohama_astc_8x8_unorm.ktx", app);
		auto textCube = gli::texture_cube{gli::load_ktx(fileContent.data(), fileContent.size())};
		memoryWithParts = device.createImageBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eTransferDst,
				e2dImageCreateInfoByTextuer(textCube, vk::ImageCreateFlagBits::eCubeCompatible),
				AAssetHander{app->activity->assetManager, "models/cube.obj.ext/mesh_0_PNt.bin"},
				AAssetHander{app->activity->assetManager, "models/cube.obj.ext/mesh_0_index_0_strip.bin"},
				sizeof(vk::DrawIndirectCommand),
				sizeof(glm::mat4));

		outputMemory = device.createBufferAndMemory(
				32,
				vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent);
		device.writeTextureToImage(textCube, std::get<vk::UniqueImage>(memoryWithParts).get());
		getUniqueImageViewTuple(memoryWithParts) = device->createImageViewUnique(
				{
						{}, std::get<vk::UniqueImage>(memoryWithParts).get(),
						vk::ImageViewType::eCube, static_cast<vk::Format >(textCube.format()),
						{
								vk::ComponentSwizzle::eR,
								vk::ComponentSwizzle::eG,
								vk::ComponentSwizzle::eB,
								vk::ComponentSwizzle::eA
						},
						{
								vk::ImageAspectFlagBits::eColor,
								0, textCube.levels(), 0, textCube.faces()
						}
				}
		);

		sampler = device->createSamplerUnique(
				vk::SamplerCreateInfo{
						{},
						vk::Filter::eLinear,
						vk::Filter::eLinear,
						vk::SamplerMipmapMode::eLinear,
						vk::SamplerAddressMode::eClampToEdge,
						vk::SamplerAddressMode::eClampToEdge,
						vk::SamplerAddressMode::eClampToEdge,
						0, 0, 0, 0, vk::CompareOp::eNever,
						0, textCube.levels(),
						vk::BorderColor::eFloatOpaqueWhite,
						0});
		createDescriptorBufferInfoTuple(memoryWithParts, 1);
		std::array descriptors{
				createDescriptorBufferInfoTuple(memoryWithParts, 2),
				createDescriptorBufferInfoTuple(memoryWithParts, 3)
		};
		vk::DescriptorImageInfo descriptorImageInfo{
				sampler.get(), getUniqueImageViewTuple(memoryWithParts).get(),
				vk::ImageLayout::eShaderReadOnlyOptimal
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[1]
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo,nullptr
				},
		};
		device->updateDescriptorSets(writeDes, nullptr);
	}

	void JobSkyBox::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {

		gcmdBuffers = helper::createCmdBuffersSub(ownerDevice(), renderPass,
		                                          *this,
		                                          swapchain.getFrameBuffer(),
		                                          swapchain.getSwapchainExtent(),
		                                          commandPool.get());
	}

	void
	JobSkyBox::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassBegin,
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
				0, std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 0).offset
		);

		cmdHandleRenderpassBegin.bindIndexBuffer(
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 1).offset,
				vk::IndexType::eUint16
		);

		cmdHandleRenderpassBegin.drawIndexed(21, 1, 0, 0, 0);

		cmdHandleRenderpassBegin.copyBuffer(
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				std::get<vk::UniqueBuffer>(outputMemory).get(),
				{vk::BufferCopy{
						createDescriptorBufferInfoTuple(memoryWithParts, 1).offset, 0,
						32}});

	}

	void
	JobSkyBox::setMVP(tt::Device &device, vk::Buffer buffer) {
		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				device->getBufferMemoryRequirements(buffer).size,
				0,
				createDescriptorBufferInfoTuple(memoryWithParts, 3).offset);
		auto outputMemoryPtr = device.mapTypeBufferMemory<uint16_t>(outputMemory);
		MY_LOG(INFO) <<createDescriptorBufferInfoTuple(memoryWithParts, 1).offset <<": "<< outputMemoryPtr[0] << ' ' << outputMemoryPtr[1] << ' ' << outputMemoryPtr[2] << ' ' << outputMemoryPtr[3] << ' ';

	}
}