//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"
#include "Device.hh"


struct Vertex {
	float pos[4];  // Position data
	float color[4];              // Color
};

namespace tt {

	JobDrawLine JobDrawLine::create(android_app *app, tt::Device &device) {
		return JobDrawLine(
				createBase(device),
				app,
				device
		);
	}

	JobBase JobDrawLine::createBase(tt::Device &device) {
		return device.createJobBase(
				{
						vk::DescriptorPoolSize{
								vk::DescriptorType::eUniformBuffer, 1
						},
						vk::DescriptorPoolSize{
								vk::DescriptorType::eStorageBuffer, 3
						}
				},
				2
		);
	}

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)},
			renderPass{createRenderpass(device)},
			compPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createComputePipeline(
								device,
								app,
								pipelineLayout);
					},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									1, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							}
					}
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
							}
					}
			} {

		std::array vertices{
				Vertex{{1.0f, 1.0f, 0.0f, 1.0f},
				       {1.0f, 1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, 1.0f, 0.0f, 1.0f},
				       {1.0f,  1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, -1.0f, 1.0f, 1.0f},
				       {1.0f,  1.0f,  1.0f, 1.0f}},
				Vertex{{1.0f, 1.0f, 1.0f, 1.0f},
				       {1.0f, 1.0f, 1.0f, 1.0f}}
		};

		//std::vector<Vertex> verticesOut{32};

		bufferMemoryPart = device.createBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eIndirectBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				vertices,
				sizeof(Vertex) * 32,
				sizeof(vk::DrawIndirectCommand));

		device.buildBufferOnBsM(
				Bsm,
				vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndirectBuffer,
				vertices,
				sizeof(Vertex) * 32,
				sizeof(vk::DrawIndirectCommand));
		//MY_LOG(INFO) << " buffer:" << sizeof(Vertex) * 32 << sizeof(Vertex) * 4;
		{
			auto localeBufferMemory = device.createStagingBufferMemoryOnObjs(vertices);

			{
				uint32_t off = 0;
				auto memoryPtr = device.mapBufferMemory(localeBufferMemory);
				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.desAndBuffers()[0], off,
				                                            vertices, sizeof(Vertex) * 32,
				                                            sizeof(vk::DrawIndirectCommand));
			}
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),
			                           Bsm.memory().get(), Bsm.size());

			//device.flushBufferTuple(localeBufferMemory,bufferparts);
		}

		outputMemory = device.createBufferAndMemory(
				sizeof(Vertex) * 32,
				vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent);

		std::array descriptors{
				createDescriptorBufferInfoTuple(bufferMemoryPart, 0),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 0)
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.desAndBuffers()[0].descriptors()[0]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.desAndBuffers()[0].descriptors()[1]
				}
		};

		device->updateDescriptorSets(writeDes, nullptr);

		auto cmdbuffers = device.createCmdBuffers(
				1, commandPool.get(),
				[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
					std::array BarrierHostWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eTransferWrite,
									vk::AccessFlagBits::eShaderRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									Bsm.desAndBuffers()[0].buffer().get(),
									0, VK_WHOLE_SIZE,
							}
					};
					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eTransfer,
							vk::PipelineStageFlagBits::eComputeShader,
							vk::DependencyFlags{},
							{},
							BarrierHostWrite, {});


					commandBufferBeginHandle.bindPipeline(vk::PipelineBindPoint::eCompute,
					                                      compPipeline.get());

					commandBufferBeginHandle.bindDescriptorSets(
							vk::PipelineBindPoint::eCompute,
							compPipeline.layout(),
							0,
							std::array{compPipeline.getDescriptorSet()},
							std::array{0u}
					);

					commandBufferBeginHandle.dispatch(32, 1, 1);

					std::array BarrierShaderWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eShaderWrite,
									vk::AccessFlagBits::eTransferRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									Bsm.desAndBuffers()[0].buffer().get(),
									0, VK_WHOLE_SIZE,
							}
					};
					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eComputeShader,
							vk::PipelineStageFlagBits::eTransfer,
							vk::DependencyFlags{},
							{},
							BarrierShaderWrite,
							{});

					commandBufferBeginHandle.copyBuffer(
							Bsm.desAndBuffers()[0].buffer().get(),
							std::get<vk::UniqueBuffer>(outputMemory).get(),
							{vk::BufferCopy{Bsm.desAndBuffers()[0].descriptors()[1].offset, 0,
							                sizeof(Vertex) * 32}});

					std::array BarrierHostRead{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eTransferWrite,
									vk::AccessFlagBits::eHostRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									std::get<vk::UniqueBuffer>(outputMemory).get(),
									0, VK_WHOLE_SIZE,
							}
					};
					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eTransfer,
							vk::PipelineStageFlagBits::eHost,
							vk::DependencyFlags{},
							{},
							BarrierHostRead,
							{});
				}
		);


		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eTransfer;
		std::array submitInfos{
				vk::SubmitInfo{
						0, nullptr, &pipelineStageFlags,
						1, &cmdbuffers[0].get(),
				}
		};
		auto renderFence = device->createFenceUnique(vk::FenceCreateInfo{});
		device.graphsQueue().submit(submitInfos, renderFence.get());
		device.waitFence(renderFence.get());
		auto outputMemoryPtr = device.mapTypeBufferMemory<Vertex>(outputMemory);
		for (int i = 0; i < 32; i++)
			MY_LOG(INFO) << outputMemoryPtr[i].pos[0];
	}


	vk::UniqueRenderPass JobDrawLine::createRenderpass(tt::Device &) {
		return vk::UniqueRenderPass();
	}

	vk::UniquePipeline JobDrawLine::createGraphsPipeline(tt::Device &device, android_app *app,
	                                                     vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/indir.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/indir.frag.spv", app);
		std::array pipelineShaderStageCreateInfos{
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
						0, sizeof(float) * 8,
						vk::VertexInputRate::eVertex
				},
				//vk::VertexInputBindingDescription{
				//		1, sizeof(glm::mat4),
				//		vk::VertexInputRate::eInstance
				//},
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32B32A32Sfloat, 16
				},
				//vk::VertexInputAttributeDescription{
				//		2, 1, vk::Format::eR32G32B32A32Sfloat, 0
				//},//VK_FORMAT_R32G32_SFLOAT
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				vk::PipelineVertexInputStateCreateFlags(),
				vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};
		//return vk::UniquePipeline{};

		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(),
		                                   device.renderPass.get(),
		                                   vk::PrimitiveTopology::eLineStrip);
	}

	vk::UniquePipeline JobDrawLine::createComputePipeline(tt::Device &device, android_app *app,
	                                                      vk::PipelineLayout pipelineLayout) {
		auto compShaderModule = device.loadShaderFromAssets("shaders/bline.comp.spv", app);

		std::array specializationMapEntrys{
				vk::SpecializationMapEntry{
						233,
						0 * sizeof(uint32_t),
						sizeof(uint32_t),
				},
				vk::SpecializationMapEntry{
						234,
						1 * sizeof(uint32_t),
						sizeof(uint32_t),
				},
				vk::SpecializationMapEntry{
						235,
						2 * sizeof(uint32_t),
						sizeof(uint32_t),
				},
		};

		std::array data{1u, 1u, 1u};

		vk::SpecializationInfo specializationInfo{
				specializationMapEntrys.size(),
				specializationMapEntrys.data(),
				data.size() * sizeof(decltype(data)::value_type), data.data()
		};

		vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eCompute,
				compShaderModule.get(),
				"main",
				&specializationInfo
		};

		vk::ComputePipelineCreateInfo computePipelineCreateInfo{
				vk::PipelineCreateFlags(),
				shaderStageCreateInfo,
				pipelineLayout
		};

		return device->createComputePipelineUnique(pipelineCache.get(), computePipelineCreateInfo);
	}

	void JobDrawLine::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {

		gcmdBuffers = helper::createCmdBuffersSub(ownerDevice(), renderPass,
		                                          *this,
		                                          swapchain.getFrameBuffer(),
		                                          swapchain.getSwapchainExtent(),
		                                          commandPool.get());

	}

	void JobDrawLine::CmdBufferRenderPassContinueBegin(
			CommandBufferBeginHandle &cmdHandleRenderpassBegin, vk::Extent2D win) {

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
				graphPipeline.get());


		cmdHandleRenderpassBegin.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.layout(),
				0,
				std::array{graphPipeline.getDescriptorSet()},
				std::array{0u}
		);

		cmdHandleRenderpassBegin.draw(32, 0, 0, 0);
	}


}