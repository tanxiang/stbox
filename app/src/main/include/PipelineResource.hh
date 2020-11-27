//
// Created by ttand on 19-12-17.
//

#ifndef STBOX_PIPELINERESOURCE_HH
#define STBOX_PIPELINERESOURCE_HH

#include "util.hh"

namespace tt {

	template<uint pipeNum = 1, uint descriptorSetNum = 1, uint descriptorSetLayoutNum = 1>
	struct gpuProgram {
	private:
		const vk::Device device;
		const vk::DescriptorPool pool;

		template<typename ... Ts>
		auto createDescriptorSetLayouts(const Ts &... objs) {
			//vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT{};
			return std::array{
					device.createDescriptorSetLayout(
							vk::DescriptorSetLayoutCreateInfo{
									vk::DescriptorSetLayoutCreateFlags(),
									objs.size(),
									objs.data()
							}
					)...
			};
		}

		vk::UniquePipelineLayout
		createPipelineLayout(vk::ArrayProxy<const vk::PushConstantRange> PushConstantRange = {}) {
			return device.createPipelineLayoutUnique(
					vk::PipelineLayoutCreateInfo{
							{},
							descriptorSetLayouts.size(),
							descriptorSetLayouts.data(),
							PushConstantRange.size(),
							PushConstantRange.data()
					}
			);
		}


		std::array<vk::DescriptorSetLayout, descriptorSetLayoutNum> descriptorSetLayouts;
		std::array<vk::DescriptorSet, descriptorSetNum> descriptorSets;//{createDescriptorSets()};
		vk::UniquePipelineLayout pipelineLayout;
		std::array<vk::UniquePipeline, pipeNum> pipeline;

		auto createDescriptorSets() {
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
					pool,
					descriptorSetLayouts.size(),
					descriptorSetLayouts.data(),
			};
			device.allocateDescriptorSets(
					&descriptorSetAllocateInfo, descriptorSets.data()
			);
		}

	public:

		auto &getDescriptorSets() {
			return descriptorSets;
		}

		auto getDescriptorSet(size_t index = 0) {
			return descriptorSets[index];
		}

		auto layout() {
			return pipelineLayout.get();
		}

		auto get(uint index = 0) {
			return pipeline[index].get();
		}

		~gpuProgram() {
			for (auto &descriptorSetLayout:descriptorSetLayouts)
				device.destroyDescriptorSetLayout(descriptorSetLayout);
			device.freeDescriptorSets(pool, descriptorSets);
		}


		template<typename ... Ts>
		gpuProgram(vk::Device dev, vk::DescriptorPool desPool,
		           vk::ArrayProxy<const vk::PushConstantRange> PushConstantRange,
		           std::function<void(std::array<vk::UniquePipeline, pipeNum> &,
		                              vk::PipelineLayout)> func,
		           const Ts &... objs)
				: device{dev}, pool{desPool},
				  descriptorSetLayouts{createDescriptorSetLayouts(objs...)},
				  pipelineLayout{createPipelineLayout(PushConstantRange)}//,
		{
			createDescriptorSets();
			func(pipeline, pipelineLayout.get());
		}


		template<typename Funcs, typename ... Ts>
		gpuProgram(vk::Device dev, vk::DescriptorPool desPool, Funcs funcs,
		           vk::ArrayProxy<const vk::PushConstantRange> PushConstantRange,
		           const Ts &... objs)
				:gpuProgram{dev, desPool, PushConstantRange,
				            [&](std::array<vk::UniquePipeline, pipeNum> &rpipelines,
				                vk::PipelineLayout rpipelineLayout) {
					            auto pipelineiter = rpipelines.begin();
					            for (auto &func:funcs) {
						            *pipelineiter++ = func(rpipelineLayout);
					            }
				            },
				            objs...} {}


		gpuProgram(gpuProgram &&) = default;
	};
}
#endif //STBOX_PIPELINERESOURCE_HH
