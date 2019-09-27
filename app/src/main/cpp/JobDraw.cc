//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "vertexdata.hh"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

namespace tt{

	static
	std::vector<vk::UniqueCommandBuffer>
	createCmdBuffers(vk::Device device,
	                 vk::RenderPass renderPass,
	                 tt::JobDraw &job,
	                 std::vector<vk::UniqueFramebuffer> &framebuffers, vk::Extent2D extent2D,
	                 vk::CommandPool pool,
	                 std::function<void(JobDraw &, RenderpassBeginHandle &,
	                                    vk::Extent2D)> functionRenderpassBegin,
	                 std::function<void(JobDraw &,CommandBufferBeginHandle &,
	                                    vk::Extent2D)> functionBegin) {
		MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << framebuffers.size();
		std::vector commandBuffers = device.allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo{
						pool,
						vk::CommandBufferLevel::ePrimary,
						framebuffers.size()
				}
		);

		std::array clearValues{
				vk::ClearValue{
						vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.2f, 0.2f}}},
				vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
		};
		uint32_t frameIndex = 0;
		for (auto &cmdBuffer : commandBuffers) {
			//cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			{
				CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
				functionBegin(job,cmdBeginHandle, extent2D);
				{
					RenderpassBeginHandle cmdHandleRenderpassBegin{
							cmdBeginHandle,
							vk::RenderPassBeginInfo{
									renderPass,
									framebuffers[frameIndex].get(),
									vk::Rect2D{
											vk::Offset2D{},
											extent2D
									},
									clearValues.size(), clearValues.data()
							}
					};
					functionRenderpassBegin(job,cmdHandleRenderpassBegin, extent2D);
				}

			}
			++frameIndex;
		}
		return commandBuffers;
	}

	JobDraw JobDraw::create(android_app *app, tt::Device &device) {
		JobDraw job{
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
				)
		};

		job.BVMs.emplace_back(
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
		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						vertices, vk::BufferUsageFlagBits::eVertexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		job.BVMs.emplace_back(
				device.createBufferAndMemoryFromVector(
						std::vector<uint32_t>{0, 1, 2, 2, 3, 0},
						vk::BufferUsageFlagBits::eIndexBuffer,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		{
			auto fileContent = loadDataFromAssets("textures/ic_launcher-web.ktx", app);
			//gli::texture2d tex2d;
			auto tex2d = gli::texture2d{gli::load_ktx(fileContent.data(), fileContent.size())};
			job.sampler = device.createSampler(tex2d.levels());
			job.IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));
		}

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);

		job.uniquePipeline = device.createPipeline(
				sizeof(decltype(vertices)::value_type),
				std::vector{
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
				},
				job.pipelineCache.get(),
				job.pipelineLayout.get()
		);

		auto descriptorBufferInfo = device.getDescriptorBufferInfo(job.BVMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(job.IVMs[0], job.sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						job.descriptorSets[0].get(), 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device.get().updateDescriptorSets(writeDes, nullptr);
//		MY_LOG(INFO)<<"jobaddr:"<<job<<std::endl;

		job.cmdbufferRenderpassBeginHandle = [](JobDraw& job,RenderpassBeginHandle &cmdHandleRenderpassBegin,
		                                        vk::Extent2D win) {
//			MY_LOG(INFO)<<"jobaddr:"<<&job<<std::endl;
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
					job.uniquePipeline.get());
			std::array tmpDescriptorSets{job.descriptorSets[0].get()};
			cmdHandleRenderpassBegin.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics,
					job.pipelineLayout.get(), 0,
					tmpDescriptorSets,
					std::vector<uint32_t>{}
			);
			vk::DeviceSize offsets[1] = {0};
			cmdHandleRenderpassBegin.bindVertexBuffers(
					0, 1,
					&std::get<vk::UniqueBuffer>(job.BVMs[1]).get(),
					offsets
			);
			cmdHandleRenderpassBegin.bindIndexBuffer(
					std::get<vk::UniqueBuffer>(job.BVMs[2]).get(),
					0, vk::IndexType::eUint32
			);
			cmdHandleRenderpassBegin.drawIndexed(6, 1, 0, 0, 0);
		};
		return job;
	}

	void JobDraw::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
//		MY_LOG(INFO)<<"jobaddr:"<<(void const *)this<<std::endl;

		cmdBuffers = createCmdBuffers(descriptorPoll.getOwner(), renderPass,
		                                      *this,
		                                      swapchain.getFrameBuffer(),
		                                      swapchain.getSwapchainExtent(),
		                                      commandPool.get(),
		                                      cmdbufferRenderpassBeginHandle,
		                                      cmdbufferCommandBufferBeginHandle);
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
};