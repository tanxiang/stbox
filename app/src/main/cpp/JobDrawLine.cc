//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"


struct Vertex {
	float pos[4];  // Position data
	float color[4];              // Color
};

namespace tt {

	JobDrawLine JobDrawLine::create(android_app *app, tt::Device &device) {
		return JobDrawLine(
				device.createJobBase(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eUniformBuffer, 1
								},
								vk::DescriptorPoolSize{
										vk::DescriptorType::eStorageBuffer, 3
								}
						},
						2
				),
				app,
				device
		);
	}

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)},
			renderPass{createRenderpass(device)},
			compPipeline{
					device.get(),
					descriptorPoll.get(),
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
					descriptorPoll.get(),
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
									1, vk::DescriptorType::eStorageBuffer,
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

		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eStorageBuffer, vertices,
		                        sizeof(Vertex) * 32);
		//MY_LOG(INFO) << " buffer:" << sizeof(Vertex) * 32 << sizeof(Vertex) * 4;
		{
			auto localeBufferMemory = device.createLocalBufferMemoryOnBsM(Bsm);

			{
				uint32_t off = 0;
				auto memoryPtr = device.mapMemorySize(
						std::get<vk::UniqueDeviceMemory>(localeBufferMemory).get(),
						device->getBufferMemoryRequirements(
								std::get<vk::UniqueBuffer>(localeBufferMemory).get()).size
				);

				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.desAndBuffers()[0], off,
				                                            vertices, sizeof(Vertex) * 32);
			}
			//Bsm.memory() = std::move(localMemory);
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),
			                           Bsm.memory().get(), Bsm.size());
		}

		outputMemory = device.createBufferAndMemory(
				sizeof(Vertex) * 32,
				vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent);

		//MY_LOG(INFO)<<"descriptorSets"<<descriptorSets.size()<<"Bsm.buffers()"<<Bsm.buffers().size();
		//MY_LOG(INFO)<<"offset="<<Bsm.buffers()[0].descriptors().size();//<<" range="<<Bsm.buffers()[0].descriptors()[0].range;
		//vk::DescriptorBufferInfo test{Bsm.buffers()[0].buffer().get(),0,128};
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
		auto outputMemoryPtr = helper::mapTypeMemoryAndSize<Vertex>(ownerDevice(), outputMemory);
		for (int i = 0; i < 32; i++)
			MY_LOG(INFO) << outputMemoryPtr[i].pos[0];
	}


	vk::UniqueRenderPass JobDrawLine::createRenderpass(tt::Device &) {
		return vk::UniqueRenderPass();
	}

	vk::UniquePipeline JobDrawLine::createGraphsPipeline(tt::Device &device, android_app *app,
	                                                     vk::PipelineLayout pipelineLayout) {

		return vk::UniquePipeline{};
		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);
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
						0, sizeof(uint32_t),
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

}