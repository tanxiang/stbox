//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"

namespace tt {

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)}, renderPass{createRenderpass(device)},
			gPipeline(createGraphsPipeline(device, app)),
			cPipeline{createComputePipeline(device, app)} {

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
						02 * sizeof(uint32_t),
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