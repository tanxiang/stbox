//
// Created by ttand on 19-11-11.
//

#include "JobAabb.hh"
#include "Device.hh"


struct Vertex {
	float pos[4];  // Position data
	float color[4];              // Color
};


struct rigidBodyData {
	std::array<float, 4> pos;//_m0;
	glm::quat quat;//_m1;
	std::array<float, 4> linVal;//_m2;
	std::array<float, 4> argVal;
	__uint32_t collidableIdx;
	float invMass;
	float restituitionCoeff;
	float frictionCoeff;
};
struct aabb {
	std::array<float, 4> min;
	std::array<float, 4> max;
};
struct collidable {
	int m_bvhIndex;
	int m_compoundBvhIndex;
	int m_shapeType;
	int m_shapeIndex;
};
namespace tt {

	JobAabb::JobAabb(android_app *app, tt::Device &device) :
			JobBase{
					device.createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									},
									vk::DescriptorPoolSize{
											vk::DescriptorType::eStorageBuffer, 6
									}
							},
							2
					)
			},
			//renderPass{createRenderpass(device)},
			compPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createComputePipeline(
								device,
								app,
								pipelineLayout);
					}, {},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									1, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									2, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									3, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									4, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									5, vk::DescriptorType::eStorageBuffer,
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
					}, {},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eVertex
							}
					}
			},
			BAM{device.createBufferAndMemory(
					sizeof(glm::mat4)*2,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent)} {

		//glm::slerp(glm::qua<float>{},glm::qua<float>{},0.3);
		auto quat0 = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		auto quat1 = glm::angleAxis(180.0f, glm::vec3(0.0f, 0.1f, 0.0f));
		auto rquat = glm::mix(quat0, quat1, 0.22f);
		std::array RigidBody{
				rigidBodyData{
						{0.0f, 0.3f, 0.0f, 0.0f},
						rquat, {}, {}, 0, {}
				},
		};
		//MY_LOG(INFO)<<"<<quat0[0]<<"<<quat0[0]<<':'<<quat0[1]<<':'<<quat0[2]<<':'<<quat0[3];
		std::array Collidables{
				collidable{0, 0, 0, 0},
				collidable{},
		};
		std::array aabbs{
				aabb{{0.0f, 0.0f, 0.0f, 1.0},
				     {0.3f, 0.4f,  0.5f,  1.0}},
				aabb{},
				aabb{},
		};
		bufferMemoryPart = device.createBufferPartsOnObjs(
				vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eUniformBuffer |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eTransferSrc |
				vk::BufferUsageFlagBits::eIndirectBuffer,
				RigidBody,
				Collidables,
				aabbs,
				sizeof(aabb) * 2,
				sizeof(Vertex) * 32,
				sizeof(vk::DrawIndirectCommand)*2,
				sizeof(glm::mat4));


		std::array descriptors{
				createDescriptorBufferInfoTuple(bufferMemoryPart, 0),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 1),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 2),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 3),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 4),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 5),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 6),
		};
		std::array writeDes{
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[0]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[1]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 2, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[2]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 3, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[3]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 4, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[4]
				},
				vk::WriteDescriptorSet{
						compPipeline.getDescriptorSet(), 5, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[5]
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[6]
				}
		};

		device->updateDescriptorSets(writeDes, nullptr);
		//auto outputMemoryPtr = device.mapTypeBufferMemory<Vertex>(outputMemory);
		//for (int i = 0; i < 32; i++)
		//	MY_LOG(INFO) << outputMemoryPtr[i].pos[0];
		cCmdbuffers = device.createCmdBuffers(
				1, commandPool.get(),
				[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
					std::array BarrierHostWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eTransferWrite,
									vk::AccessFlagBits::eShaderRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
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

					commandBufferBeginHandle.dispatch(1, 1, 1);

					std::array BarrierShaderWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eShaderWrite,
									vk::AccessFlagBits::eTransferRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
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

					/*commandBufferBeginHandle.copyBuffer(
							std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
							std::get<vk::UniqueBuffer>(outputMemory).get(),
							{vk::BufferCopy{
									createDescriptorBufferInfoTuple(bufferMemoryPart, 1).offset, 0,
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
							{});					 */

				},{},vk::CommandBufferLevel::eSecondary
		);
		/*
		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eTransfer;
		std::array submitInfos{
				vk::SubmitInfo{
						0, nullptr, &pipelineStageFlags,
						1, &cmdbuffers[0].get(),
				}
		};
		auto renderFence = device->createFenceUnique(vk::FenceCreateInfo{});
		device.graphsQueue().submit(submitInfos, renderFence.get());
		device.waitFence(renderFence.get());*/
	}


	//vk::UniqueRenderPass JobAabb
	//::createRenderpass(tt::Device &) {
	//	return vk::UniqueRenderPass();
	//}

	vk::UniquePipeline JobAabb::createGraphsPipeline(tt::Device &device, android_app *app,
	                                                 vk::PipelineLayout pipelineLayout) {


		auto vertShaderModule = device.loadShaderFromAssets("shaders/indir.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/indir.frag.spv", app);
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
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32B32A32Sfloat, 16
				}
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()
		};
		//return vk::UniquePipeline{};

		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(),
		                                   device.renderPass.get(),
		                                   vk::PrimitiveTopology::eTriangleFan);
	}

	vk::UniquePipeline JobAabb::createComputePipeline(tt::Device &device, android_app *app,
	                                                  vk::PipelineLayout pipelineLayout) {
		auto compShaderModule = device.loadShaderFromAssets("shaders/updateAabbs.comp.spv", app);

		std::array specializationMapEntrys{
				vk::SpecializationMapEntry{
						0,
						0 * sizeof(uint32_t),
						sizeof(uint32_t),
				},
				vk::SpecializationMapEntry{
						1,
						1 * sizeof(uint32_t),
						sizeof(uint32_t),
				},
				vk::SpecializationMapEntry{
						2,
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
				{},
				vk::ShaderStageFlagBits::eCompute,
				compShaderModule.get(),
				"main",
				&specializationInfo
		};

		vk::ComputePipelineCreateInfo computePipelineCreateInfo{
				{},
				shaderStageCreateInfo,
				pipelineLayout
		};

		return device->createComputePipelineUnique(pipelineCache.get(), computePipelineCreateInfo);
	}

	void JobAabb::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {

		gcmdBuffers = helper::createCmdBuffersSub(ownerDevice(), renderPass,
		                                          *this,
		                                          swapchain.getFrameBuffer(),
		                                          swapchain.getSwapchainExtent(),
		                                          commandPool.get());

	}

	void JobAabb::CmdBufferRenderPassContinueBegin(
			CommandBufferBeginHandle &cmdHandleRenderpassBegin, vk::Extent2D win,
			uint32_t frameIndex) {
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
				0, {std::get<vk::UniqueBuffer>(bufferMemoryPart).get()},
				{createDescriptorBufferInfoTuple(bufferMemoryPart, 4).offset}
		);

		cmdHandleRenderpassBegin.drawIndirect(std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
		                                      createDescriptorBufferInfoTuple(bufferMemoryPart, 5).offset, 2,
		                                      sizeof(vk::DrawIndirectCommand));
		//cmdHandleRenderpassBegin.draw(8, 1, 0, 0);
	}

	void JobAabb::setMVP(tt::Device &device) {
		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAM);
			memory_ptr[0] = perspective * lookat;

		}
		auto buffer = std::get<vk::UniqueBuffer>(BAM).get();

		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				sizeof(glm::mat4),
				0,
				createDescriptorBufferInfoTuple(bufferMemoryPart, 6).offset);

		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<rigidBodyData>(ownerDevice(), BAM);
			memory_ptr[0] = rigidBodyData{
					{0.0f, -0.3f, 0.0f, 0.0f},
					fRotate, {}, {}, 0, {}
			};

		}

		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				sizeof(rigidBodyData),
				0,
				createDescriptorBufferInfoTuple(bufferMemoryPart, 0).offset);
	}


}