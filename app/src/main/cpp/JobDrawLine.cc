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

	vk::UniquePipeline JobDrawLine::createComputePipeline(tt::Device &, android_app *app) {
		return vk::UniquePipeline();
	}

}