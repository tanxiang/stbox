//
// Created by ttand on 19-12-17.
//

#ifndef STBOX_PIPELINERESOURCE_HH
#define STBOX_PIPELINERESOURCE_HH

#include "util.hh"

namespace tt {


	struct PipelineResource {
	private:
		const vk::Device device;
		const vk::DescriptorPool pool;
		template<typename ... Ts>
		auto createDescriptorSetLayouts(const Ts &... objs) {
			return std::vector{
					device.createDescriptorSetLayout(
							vk::DescriptorSetLayoutCreateInfo{
									vk::DescriptorSetLayoutCreateFlags(),
									objs.size(),
									objs.data()
							}
					)...
			};
		}

		std::vector<vk::DescriptorSet> createDescriptorSets() {
			return device.allocateDescriptorSets(
					vk::DescriptorSetAllocateInfo{
							pool,
							descriptorSetLayouts.size(),
							descriptorSetLayouts.data(),
					}
			);
		}

		vk::UniquePipelineLayout createPipelineLayout() {
			return device.createPipelineLayoutUnique(
					vk::PipelineLayoutCreateInfo{
							vk::PipelineLayoutCreateFlags(),
							descriptorSetLayouts.size(),
							descriptorSetLayouts.data(),
							0,
							nullptr
					}
			);
		}

	public:

		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		std::vector<vk::DescriptorSet> descriptorSets{createDescriptorSets()};
		vk::UniquePipelineLayout pipelineLayout{createPipelineLayout()};
		vk::UniquePipeline pipeline;

		~PipelineResource() {
			for (auto &descriptorSetLayout:descriptorSetLayouts)
				device.destroyDescriptorSetLayout(descriptorSetLayout);
			device.freeDescriptorSets(pool, descriptorSets);
		}

		template<typename ... Ts>
		PipelineResource(vk::Device dev, vk::DescriptorPool desPool,const Ts &... objs)
				: device{dev}, pool{desPool},
				descriptorSetLayouts{createDescriptorSetLayouts(objs...)} {}
	};
}
#endif //STBOX_PIPELINERESOURCE_HH
