//
// Created by ttand on 20-3-2.
//

#include "JobSkyBox.hh"
#include "Device.hh"
#include <gli/gli.hpp>

namespace tt {

	vk::UniquePipeline JobSkyBox::createGraphsPipeline(tt::Device &, android_app *app,
	                                                   vk::PipelineLayout pipelineLayout) {
		return vk::UniquePipeline();
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
		//MY_LOG(INFO)<<"textures/cubemap_yokohama_astc_8x8_unorm.ktx\n\t="<<vk::to_string(e2dImageCreateInfoByTextuer(textCube,vk::ImageCreateFlagBits::eCubeCompatible).format);
		BufferImageMemoryWithParts<3> bufferMemoryPart = device.createImageBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer |
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				e2dImageCreateInfoByTextuer(textCube,vk::ImageCreateFlagBits::eCubeCompatible),
				sizeof(Vertex) * 32,
				sizeof(vk::DrawIndirectCommand),
				sizeof(glm::mat4));



	}

	void JobSkyBox::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {

		gcmdBuffers = helper::createCmdBuffersSub(ownerDevice(), renderPass,
		                                          *this,
		                                          swapchain.getFrameBuffer(),
		                                          swapchain.getSwapchainExtent(),
		                                          commandPool.get());
	}

	void JobSkyBox::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleBegin,
	                                                 vk::Extent2D win, uint32_t frameIndex) {

	}
}