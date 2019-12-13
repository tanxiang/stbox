//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"


struct Vertex {
	float pos[4];  // Position data
	float color[4];              // Color
};

namespace tt {

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)}, renderPass{createRenderpass(device)},
			gPipeline(createGraphsPipeline(device, app)),
			cPipeline{createComputePipeline(device, app)} {

		std::array vertices{
				Vertex{{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, 1.0f, 0.0f, 1.0f}, {1.0f,  1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, -1.0f, 1.0f, 1.0f}, {1.0f,  1.0f,  1.0f, 1.0f}},
				Vertex{{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
		};

		//std::vector<Vertex> verticesOut{32};

		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eStorageBuffer, vertices,
		                        sizeof(Vertex)*32);
		{
			auto localeBufferMemory = device.createLocalBufferMemoryOnBsM(Bsm);

			{
				uint32_t off = 0;
				auto memoryPtr = device.mapMemorySize(
						std::get<vk::UniqueDeviceMemory>(localeBufferMemory).get(),
						device->getBufferMemoryRequirements(
								std::get<vk::UniqueBuffer>(localeBufferMemory).get()).size
				);

				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.buffers()[0], off,
				                                            vertices, sizeof(Vertex)*32);
			}
			//Bsm.memory() = std::move(localMemory);
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),
			                           Bsm.memory().get(), Bsm.size());
		}

		outputMemory = device.createBufferAndMemory(
				sizeof(Vertex)*32,
				vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eHostVisible|
				vk::MemoryPropertyFlagBits::eHostCoherent);

		//MY_LOG(INFO)<<"descriptorSets"<<descriptorSets.size()<<"Bsm.buffers()"<<Bsm.buffers().size();
		//MY_LOG(INFO)<<"offset="<<Bsm.buffers()[0].descriptors().size();//<<" range="<<Bsm.buffers()[0].descriptors()[0].range;
		//vk::DescriptorBufferInfo test{Bsm.buffers()[0].buffer().get(),0,128};
		std::array writeDes{
				vk::WriteDescriptorSet{
						descriptorSets[1].get(), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.buffers()[0].descriptors()[0]
				},
				vk::WriteDescriptorSet{
						descriptorSets[1].get(), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.buffers()[0].descriptors()[1]
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
									Bsm.buffers()[0].buffer().get(),
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
					                                      cPipeline.get());

					commandBufferBeginHandle.bindDescriptorSets(
							vk::PipelineBindPoint::eCompute,
							pipelineLayouts[0].get(),
							0,
							std::array{descriptorSets[1].get()},
							std::array{0u}
					);

					commandBufferBeginHandle.dispatch(32,1,1);

					std::array BarrierShaderWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eShaderWrite,
									vk::AccessFlagBits::eTransferRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									Bsm.buffers()[0].buffer().get(),
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
							Bsm.buffers()[0].buffer().get(),
							std::get<vk::UniqueBuffer>(outputMemory).get(),
							{vk::BufferCopy{Bsm.buffers()[0].descriptors()[1].offset, 0, sizeof(Vertex)*32}});

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
		auto outputMemoryPtr = helper::mapTypeMemoryAndSize<Vertex>(ownerDevice(),outputMemory);
		for(int i =0;i<32;i++)
			MY_LOG(INFO)<<outputMemoryPtr[i].pos[0];
	}

	JobDrawLine JobDrawLine::create(android_app *app, tt::Device &device) {
		return JobDrawLine(
				device.createJob(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eUniformBuffer, 1
								},
								vk::DescriptorPoolSize{
										vk::DescriptorType::eStorageBuffer, 3
								}
						},
						{
								{
										vk::DescriptorSetLayoutBinding{
												0, vk::DescriptorType::eUniformBuffer,
												1, vk::ShaderStageFlagBits::eVertex
										},
										{
												1, vk::DescriptorType::eStorageBuffer,
												1, vk::ShaderStageFlagBits::eVertex
										}
								},
								{
										{
												0, vk::DescriptorType::eStorageBuffer,
												1, vk::ShaderStageFlagBits::eCompute
										},
										{
												1, vk::DescriptorType::eStorageBuffer,
												1, vk::ShaderStageFlagBits::eCompute
										}
								}
						}
				),
				app,
				device
		);
	}


	vk::UniqueRenderPass JobDrawLine::createRenderpass(tt::Device &) {
		return vk::UniqueRenderPass();
	}

	vk::UniquePipeline JobDrawLine::createGraphsPipeline(tt::Device &, android_app *app) {
		return vk::UniquePipeline();
	}

	vk::UniquePipeline JobDrawLine::createComputePipeline(tt::Device &device, android_app *app) {
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
				pipelineLayouts[1].get()
		};

		return device->createComputePipelineUnique(pipelineCache.get(), computePipelineCreateInfo);
	}

}