//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "vertexdata.hh"
#include "Device.hh"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

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

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);
		std::array pipelineShaderStageCreateInfos
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
		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(), device.renderPass.get());

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
		helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAMs[0])[0] =
				perspective * glm::lookAt(
						camPos,  // Camera is at (-5,3,-10), in World Space
						camTo,     // and looks at the origin
						camUp     // Head is up (set to 0,-1,0 to look upside-down)
				);
	}

	void JobDraw::CmdBufferRenderpassBegin(RenderpassBeginHandle &cmdHandleRenderpassBegin,
	                                       vk::Extent2D win) {

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

		cmdHandleRenderpassBegin.setScissor(0, std::array{vk::Rect2D{vk::Offset2D{}, win}});
		cmdHandleRenderpassBegin.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.pipeline.get());
		//std::array tmpDescriptorSets{descriptorSets[0].get()};
		cmdHandleRenderpassBegin.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.pipelineLayout.get(), 0,
				graphPipeline.descriptorSets,
				{});
		std::array offsets{vk::DeviceSize{0}};
		//vk::DeviceSize offsets[1] = {0};
		cmdHandleRenderpassBegin.bindVertexBuffers(
				0,
				//std::get<vk::UniqueBuffer>(BAMs[1]).get(),
				Bsm.desAndBuffers()[0].buffer().get(),
				offsets);
		cmdHandleRenderpassBegin.bindIndexBuffer(
				Bsm.desAndBuffers()[1].buffer().get(),
				0, vk::IndexType::eUint32);
		cmdHandleRenderpassBegin.drawIndexed(6, 1, 0, 0, 0);
	}

	JobDraw::JobDraw(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)},
			graphPipeline{
					device.get(),
					descriptorPoll.get(),
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
			} {
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
		std::array indexes{0u, 1u, 2u, 2u, 3u, 0u};
		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eVertexBuffer, vertices);
		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eIndexBuffer, indexes);
		{
			auto localeBufferMemory = device.createLocalBufferMemoryOnBsM(Bsm);

			{
				uint32_t off = 0;
				auto memoryPtr = device.mapMemorySize(
						std::get<vk::UniqueDeviceMemory>(localeBufferMemory).get(),
						device->getBufferMemoryRequirements(
								std::get<vk::UniqueBuffer>(localeBufferMemory).get()).size
				);
				//Bsm.buffers()[0].descriptors() =
				//		device.writeObjs(memoryPtr, Bsm.buffers()[0].buffer().get(), off, vertices);

				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.desAndBuffers()[0], off,
				                                            vertices);
				//MY_LOG(INFO)<<"descriptors:" << Bsm.buffers()[0].descriptors().size() <<" off:"<<Bsm.buffers()[0].descriptors()[0].offset
				//<<" size:"<<Bsm.buffers()[0].descriptors()[0].range;
				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.desAndBuffers()[1], off,
				                                            indexes);

				//Bsm.buffers()[1].descriptors() =
				//		device.writeObjs(memoryPtr, Bsm.buffers()[1].buffer().get(), off, indexes);
			}
			//Bsm.memory() = std::move(localMemory);
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),
			                           Bsm.memory().get(), Bsm.size());
		}

		{
			auto fileContent = loadDataFromAssets("textures/ic_launcher-web.ktx", app);
			//gli::texture2d tex2d;
			auto tex2d = gli::texture2d{gli::load_ktx(fileContent.data(), fileContent.size())};
			sampler = device.createSampler(tex2d.levels());
			IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));
		}


		//uniquePipeline = createPipeline(device, app);

		auto descriptorBufferInfo = device.getDescriptorBufferInfo(BAMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(IVMs[0], sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.descriptorSets[0], 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						graphPipeline.descriptorSets[0], 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);
	}
};