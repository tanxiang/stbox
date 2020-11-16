//
// Created by ttand on 19-11-11.
//

#include "JobAabb.hh"
#include "Device.hh"


struct rigidBodyData {
	std::array<float, 4> pos;//_m0;
	glm::quat quat;//_m1;
	std::array<float, 4> linVal;//_m2;
	glm::quat argVal;
	__uint32_t collidableIdx;
	__uint32_t invMassIdx;
	__uint32_t LocalAabbIdx;
	__uint32_t ext0;
};
struct aabb {
	std::array<float, 4> min;
	std::array<float, 4> max;
};

struct invariant {
	float Mass;
	float restituitionCoeff;
	float frictionCoeff;
	float otCoeff;
	glm::mat3 Inertia;
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
											vk::DescriptorType::eUniformBuffer, 2
									},
									vk::DescriptorPoolSize{
											vk::DescriptorType::eStorageBuffer, 12
									}
							},
							4
					)
			},
			compMprPipeline{
					device.get(),
					descriptorPool.get(),
					{},
					[&](std::array<vk::UniquePipeline, 2> &pipelines,
					    vk::PipelineLayout pipelineLayout) {
						createComputePipeline(
								pipelines,
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
							},
							vk::DescriptorSetLayoutBinding{
									6, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							},
							vk::DescriptorSetLayoutBinding{
									7, vk::DescriptorType::eStorageBuffer,
									1, vk::ShaderStageFlagBits::eCompute
							}

					}, std::array{
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
					}
			},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					{},
					[&](std::array<vk::UniquePipeline, 2> &pipelines,
					    vk::PipelineLayout pipelineLayout) {
						createGraphsPipelines(pipelines,
						                      device,
						                      app,
						                      pipelineLayout);
					},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eGeometry
							}
					}
			},
			BAM{device.createBufferAndMemory(
					sizeof(glm::mat4) * 2,
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent)} {

		//glm::slerp(glm::qua<float>{},glm::qua<float>{},0.3);
		auto quat0 = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
		auto quat1 = glm::angleAxis(180.0f, glm::vec3(0.0f, 0.1f, 0.0f));
		auto rquat = glm::mix(quat0, quat1, 0.22f);
		std::array RigidBody{
				rigidBodyData{
						{0.0f, 0.3f, 0.0f, 0.0f}, rquat,
						{}, {},
						0, 0, 0, 0
				},
				rigidBodyData{
						{0.1f, 0.1f, -0.4f, 0.0f}, rquat,
						{}, {},
						0, 0, 0, 0
				}
		};
		//MY_LOG(INFO)<<"<<quat0[0]<<"<<quat0[0]<<':'<<quat0[1]<<':'<<quat0[2]<<':'<<quat0[3];
		std::array Collidables{
				collidable{0, 0, 0, 0},
				collidable{},
		};
		std::array aabbs{
				aabb{{0.0f, 0.0f, 0.0f, 1.0},
				     {0.3f, 0.4f, 0.5f, 1.0}},
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
				sizeof(uint32_t)*4,
				sizeof(vk::DispatchIndirectCommand) * 2,
				sizeof(float) * 128,
				sizeof(vk::DrawIndirectCommand) * 2,
				sizeof(float) * 128,
				sizeof(vk::DrawIndirectCommand) * 2,
				sizeof(float) * 128,
				sizeof(vk::DrawIndirectCommand) * 2,
				sizeof(glm::mat4));//pair


		auto descriptors = bufferMemoryPart.arrayOfDescs();

		std::array writeDes{
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[0]
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[1]
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(), 2, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[2]//in aabb
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(), 3, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[3]//out aabb
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(), 4, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[4]//pair
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(1), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[8]
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(1), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[9]
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(1), 2, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[10]
				},
				vk::WriteDescriptorSet{
						compMprPipeline.getDescriptorSet(1), 3, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &descriptors[11]
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptors[12]
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
					                                      compMprPipeline.get());

					commandBufferBeginHandle.bindDescriptorSets(
							vk::PipelineBindPoint::eCompute,
							compMprPipeline.layout(),
							0,
							compMprPipeline.getDescriptorSets(),
							std::array{0u}
					);

					commandBufferBeginHandle.dispatch(2, 1, 1);

					std::array BarrierShaderWrite{
							vk::BufferMemoryBarrier{
									vk::AccessFlagBits::eShaderWrite,
									vk::AccessFlagBits::eShaderRead,
									VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
									std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
									0, VK_WHOLE_SIZE,
							}
					};
					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eComputeShader,
							vk::PipelineStageFlagBits::eComputeShader,
							vk::DependencyFlags{},
							{},
							BarrierShaderWrite,
							{});

					//commandBufferBeginHandle.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

					commandBufferBeginHandle.bindPipeline(vk::PipelineBindPoint::eCompute,
					                                      compMprPipeline.get(1));
					commandBufferBeginHandle.bindDescriptorSets(
							vk::PipelineBindPoint::eCompute,
							compMprPipeline.layout(),
							0,
							compMprPipeline.getDescriptorSets(),
							std::array{0u}
					);
					commandBufferBeginHandle.dispatch(1, 1, 1);
/*
					commandBufferBeginHandle.copyBuffer(
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

				}, {}, vk::CommandBufferLevel::eSecondary
		);
	}

	void
	JobAabb::createGraphsPipelines(std::array<vk::UniquePipeline, 2> &pipelines, Device &device,
	                               android_app *app, vk::PipelineLayout pipelineLayout) {
		auto vertShaderModule = device.loadShaderFromAssets("shaders/copy0.vert.spv", app);
		auto gemoShaderModule = device.loadShaderFromAssets("shaders/aabb.geom.spv", app);
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
						vk::ShaderStageFlagBits::eGeometry,
						gemoShaderModule.get(),
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
						0, sizeof(float) * 4,
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				}
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()
		};

		pipelines[0] = device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                           pipelineVertexInputStateCreateInfo,
		                                           pipelineLayout,
		                                           pipelineCache.get(),
		                                           device.renderPass.get(),
		                                           vk::PrimitiveTopology::eTriangleList,
		                                           vk::PolygonMode::eLine);

		auto gemocubeShaderModule = device.loadShaderFromAssets("shaders/cube.geom.spv", app);

		pipelineShaderStageCreateInfos[1] = vk::PipelineShaderStageCreateInfo{
				{},
				vk::ShaderStageFlagBits::eGeometry,
				gemocubeShaderModule.get(),
				"main"
		};
		pipelines[1] = device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                           pipelineVertexInputStateCreateInfo,
		                                           pipelineLayout,
		                                           pipelineCache.get(),
		                                           device.renderPass.get(),
		                                           vk::PrimitiveTopology::eLineListWithAdjacency);
	}

	void
	JobAabb::createComputePipeline(std::array<vk::UniquePipeline, 2> &pipelines, tt::Device &device,
	                               android_app *app,
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

		vk::ComputePipelineCreateInfo computePipelineCreateInfo{
				{},
				{
						{},
						vk::ShaderStageFlagBits::eCompute,
						compShaderModule.get(),
						"main",
						&specializationInfo
				},
				pipelineLayout
		};

		pipelines[0] = device->createComputePipelineUnique(pipelineCache.get(),
		                                                   computePipelineCreateInfo);

		auto compMprShaderModule = device.loadShaderFromAssets("shaders/mpr.comp.spv", app);

		//shaderStageCreateInfo.module = compMprShaderModule.get();

		computePipelineCreateInfo.stage.module =  compMprShaderModule.get();
		pipelines[1] = device->createComputePipelineUnique(pipelineCache.get(),
		                                                   computePipelineCreateInfo);

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
				{createDescriptorBufferInfoTuple(bufferMemoryPart, 8).offset}
		);

		cmdHandleRenderpassBegin.drawIndirect(
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 9).offset,
				1,
				sizeof(vk::DrawIndirectCommand));
		//cmdHandleRenderpassBegin.draw(8, 1, 0, 0);

		cmdHandleRenderpassBegin.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.get(1));

//		cmdHandleRenderpassBegin.bindDescriptorSets(
//				vk::PipelineBindPoint::eGraphics,
//				graphPipeline.layout(), 0,
//				graphPipeline.getDescriptorSets(),
//				{}
//		);

		cmdHandleRenderpassBegin.bindVertexBuffers(
				0, {std::get<vk::UniqueBuffer>(bufferMemoryPart).get()},
				{createDescriptorBufferInfoTuple(bufferMemoryPart, 10).offset}
		);

		cmdHandleRenderpassBegin.drawIndirect(
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				createDescriptorBufferInfoTuple(bufferMemoryPart, 11).offset,
				2,
				sizeof(vk::DrawIndirectCommand));

	}

	void JobAabb::setMVP(tt::Device &device) {
		{
			auto memory_ptr = helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAM);
			memory_ptr[0] = perspective * lookat * glm::mat4_cast(fRotate);

		}
		auto buffer = std::get<vk::UniqueBuffer>(BAM).get();

		device.flushBufferToBuffer(
				buffer,
				std::get<vk::UniqueBuffer>(bufferMemoryPart).get(),
				sizeof(glm::mat4),
				0,
				createDescriptorBufferInfoTuple(bufferMemoryPart, 12).offset);

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