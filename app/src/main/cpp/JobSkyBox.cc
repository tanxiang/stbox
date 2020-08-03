//
// Created by ttand on 20-3-2.
//

#include "JobSkyBox.hh"
#include "Device.hh"
//#include <gli/gli.hpp>
//#include "model.hh"
#include "ktx2.hh"
//#include "aopen.h"

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
					}, {},
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
			},
			BAM{device.createBufferAndMemory(
					sizeof(glm::mat4),
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent)} {

		//auto fileContent = loadDataFromAssets("textures/cubemap_yokohama_astc_8x8_unorm.ktx", app);
		//auto textCube = gli::texture_cube{gli::load_ktx((char*)fileContent.data(), fileContent.size())};
		//AAssetHander ktx2{app->activity->assetManager, "textures/cube_bcmp.ktx"};
		auto ktx2fileContent = loadDataFromAssets("textures/cube_bcmp.ktx", app);
		ktx2 ktx2texture{ktx2fileContent.data(), ktx2fileContent.size()};

		//ktx2texture.debugLoad(device.phyDevice(),device.get(),device.graphsQueue(),commandPool.get());

		memoryWithParts = device.createImageBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eTransferDst,
				//e2dImageCreateInfoByTextuer(textCube, vk::ImageCreateFlagBits::eCubeCompatible),
				ktx2texture.vkImageCI(),
				AAssetHander{app->activity->assetManager, "models/cube.obj.ext/mesh_0_P.bin"},
				AAssetHander{app->activity->assetManager,
				             "models/cube.obj.ext/mesh_0_index_0_strip.bin"},
				AAssetHander{app->activity->assetManager,
				             "models/cube.obj.ext/mesh_0_index_0_strip_draw.bin"},
				sizeof(glm::mat4));

		//device.writeTextureToImage(textCube, std::get<vk::UniqueImage>(memoryWithParts).get());
		device.writeTextureToImage(ktx2texture, std::get<vk::UniqueImage>(memoryWithParts).get());
		getUniqueImageViewTuple(memoryWithParts) = device->createImageViewUnique(
				{
						{}, std::get<vk::UniqueImage>(memoryWithParts).get(),
						vk::ImageViewType::eCube, ktx2texture.format(),
						{
								vk::ComponentSwizzle::eR,
								vk::ComponentSwizzle::eG,
								vk::ComponentSwizzle::eB,
								vk::ComponentSwizzle::eA
						},
						{
								vk::ImageAspectFlagBits::eColor,
								0, ktx2texture.numLevels(), 0, ktx2texture.numLayersAll()
						}
				}
				//ktx2texture.vkImageViewCI(std::get<vk::UniqueImage>(memoryWithParts).get())
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
						0,
						//textCube.levels(),
						ktx2texture.numLevels(),
						vk::BorderColor::eFloatOpaqueWhite,
						0});

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
						&descriptorImageInfo, nullptr
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

		cmdHandleRenderpassBegin.drawIndexedIndirect(
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				createDescriptorBufferInfoTuple(memoryWithParts, 2).offset,
				1, 0);
	}

	void
	JobSkyBox::setMVP(tt::Device &device) {
		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAM);

			static auto trans = glm::translate(glm::mat4{1.0}, glm::vec3(0, 0, -4));
			memory_ptr[0] = perspective * lookat * trans * glm::mat4_cast(fRotate);
		}
		auto buffer = std::get<vk::UniqueBuffer>(BAM).get();
		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(memoryWithParts).get(),
				device->getBufferMemoryRequirements(buffer).size,
				0,
				createDescriptorBufferInfoTuple(memoryWithParts, 3).offset);
	}
}