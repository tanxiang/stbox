//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "vertexdata.hh"
#include "Device.hh"

namespace tt {

	JobDraw JobDraw::create(android_app *app, tt::Device &device) {
		return JobDraw{
				device.createJobBase(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eUniformBuffer, 1
								},
								vk::DescriptorPoolSize{
										vk::DescriptorType::eCombinedImageSampler, 1
								},
						}, 1
				),
				app,
				device
		};
	}

	vk::UniquePipeline
	JobDraw::createPipeline(Device &device, android_app *app, vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/copy.vert.spv", app);
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




	void JobDraw::setPv() {
		helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAMs[0])[0] =
				perspective * lookat * glm::mat4_cast(fRotate) ;
	}

	JobDraw::JobDraw(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createPipeline(device, app, pipelineLayout);
					},{},
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
		BAMs.emplace_back(
				device.createBufferAndMemory(
						sizeof(glm::mat4),
						vk::BufferUsageFlagBits::eUniformBuffer |
						vk::BufferUsageFlagBits::eTransferSrc,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));
		return;
	}

	void
	JobDraw::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassContinue,
	                                          vk::Extent2D win, uint32_t frameIndex) {
		return;

	}
};