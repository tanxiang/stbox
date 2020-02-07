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
							0,//FIXME input
							nullptr
					}
			);
		}


		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		std::vector<vk::DescriptorSet> descriptorSets{createDescriptorSets()};
		vk::UniquePipelineLayout pipelineLayout{createPipelineLayout()};
		vk::UniquePipeline pipeline;
	public:

		auto& getDescriptorSets(){
			return descriptorSets;
		}

		auto getDescriptorSet(size_t index = 0){
			return descriptorSets[index];
		}


		auto layout(){
			return pipelineLayout.get();
		}

		auto get(){
			return pipeline.get();
		}

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


		PipelineResource(PipelineResource &&) = default;
	};
}
#endif //STBOX_PIPELINERESOURCE_HH
