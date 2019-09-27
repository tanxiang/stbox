//
// Created by ttand on 19-9-26.
//

#include "JobFont.hh"

tt::JobFont tt::JobFont::create(android_app *app, tt::Device &device) {
	JobFont job{
			device.createJob(
					{
							vk::DescriptorPoolSize{
									vk::DescriptorType::eCombinedImageSampler, 1
							}
					},
					{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eCombinedImageSampler,
									1, vk::ShaderStageFlagBits::eFragment
							}
					}
			)
	};
	job.BVMs.emplace_back(
			device.createBufferAndMemoryFromAssets(
					app, {"glyhps/glyphy_3072.bin","glyhps/cell_21576.bin","glyhps/point_47440.bin"},
					vk::BufferUsageFlagBits::eStorageBuffer,
					vk::MemoryPropertyFlagBits::eDeviceLocal));

	return job;
}
