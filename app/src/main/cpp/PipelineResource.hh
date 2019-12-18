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


		auto createDescriptorSetLayouts(
				std::initializer_list<vk::ArrayProxy<vk::DescriptorSetLayoutBinding>> objs) {
			std::vector<vk::DescriptorSetLayout> dSetLayouts;
			for (auto &obj:objs)
				dSetLayouts.emplace_back(device.createDescriptorSetLayout(
						vk::DescriptorSetLayoutCreateInfo{
								vk::DescriptorSetLayoutCreateFlags(),
								obj.size(),
								obj.data()
						}));
			return dSetLayouts;
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
							0,//FIXME input
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

		template<typename Func,typename ... Ts>
		PipelineResource(vk::Device dev, vk::DescriptorPool desPool,Func func,const Ts &... objs)
				: device{dev}, pool{desPool},
				  descriptorSetLayouts{createDescriptorSetLayouts(objs...)},
				  pipeline{func(pipelineLayout.get())} {}


		PipelineResource(vk::Device dev, vk::DescriptorPool desPool,
		                 std::initializer_list<vk::ArrayProxy<vk::DescriptorSetLayoutBinding>> objs)
				: device{dev}, pool{desPool},
				descriptorSetLayouts{createDescriptorSetLayouts(objs)} {}

		PipelineResource(PipelineResource &&) = default;
	};
}
#endif //STBOX_PIPELINERESOURCE_HH
