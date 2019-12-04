//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"


struct Vertex {
	float pos[4];  // Position data
	float c[4];              // Color
};

namespace tt {

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)}, renderPass{createRenderpass(device)},
			gPipeline(createGraphsPipeline(device, app)),
			cPipeline{createComputePipeline(device, app)} {

		std::array vertices{
				Vertex{{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, 1.0f,  0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
				Vertex{{-1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
				Vertex{{1.0f,  1.0f,  1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
		};

		std::vector<Vertex> verticesOut{30};

		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eStorageBuffer, vertices,verticesOut);
		{
			auto localeBufferMemory = device.createLocalBufferMemoryOnBsM(Bsm);

			{
				uint32_t off = 0;
				auto memoryPtr = device.mapMemorySize(
						std::get<vk::UniqueDeviceMemory>(localeBufferMemory).get(),
						device->getBufferMemoryRequirements(std::get<vk::UniqueBuffer>(localeBufferMemory).get()).size
				);

				off += device.writeObjsDescriptorBufferInfo(memoryPtr, Bsm.buffers()[0], off, vertices,verticesOut);
			}
			//Bsm.memory() = std::move(localMemory);
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),Bsm.memory().get(), Bsm.size());
		}


		std::array writeDes{
				vk::WriteDescriptorSet{
						descriptorSets[0].get(), 0, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.buffers()[0].descriptors()[0]
				},
				vk::WriteDescriptorSet{
						descriptorSets[0].get(), 1, 0, 1,
						vk::DescriptorType::eStorageBuffer,
						nullptr, &Bsm.buffers()[0].descriptors()[1]
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);

		cPipeline = createComputePipeline(device,app);


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

		std::array data{ 1u, 1u, 1u };

		vk::SpecializationInfo specializationInfo{
				specializationMapEntrys.size(),
				specializationMapEntrys.data(),
				data.size() * sizeof(decltype(data)::value_type),data.data()
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