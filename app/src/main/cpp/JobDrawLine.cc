//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"

namespace tt {

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)} {


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
								vk::DescriptorSetLayoutBinding{
										0, vk::DescriptorType::eUniformBuffer,
										1, vk::ShaderStageFlagBits::eVertex
								},
								{
										1, vk::DescriptorType::eStorageBuffer,
										1, vk::ShaderStageFlagBits::eVertex
								},
								{
										2, vk::DescriptorType::eStorageBuffer,
										1, vk::ShaderStageFlagBits::eCompute
								},
						}
				),
				app,
				device
		);
	}
}