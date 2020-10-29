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
			vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT{};
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


		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		std::vector<vk::DescriptorSet> descriptorSets{createDescriptorSets()};
		vk::UniquePipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;
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

		auto get() {
			return pipeline.get();
		}

		~PipelineResource() {
			for (auto &descriptorSetLayout:descriptorSetLayouts)
				device.destroyDescriptorSetLayout(descriptorSetLayout);
			device.freeDescriptorSets(pool, descriptorSets);
		}

		template<typename Func, typename ... Ts>
		PipelineResource(vk::Device dev, vk::DescriptorPool desPool, Func func,
		                 vk::ArrayProxy<const vk::PushConstantRange> PushConstantRange,
		                 const Ts &... objs)
				: device{dev}, pool{desPool},
				  descriptorSetLayouts{createDescriptorSetLayouts(objs...)},
				  pipelineLayout{createPipelineLayout(PushConstantRange)},
				  pipeline{func(pipelineLayout.get())} {}


		PipelineResource(PipelineResource &&) = default;
	};

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
					&descriptorSetAllocateInfo,descriptorSets.data()
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

		template<typename Funcs, typename ... Ts>
		gpuProgram(vk::Device dev, vk::DescriptorPool desPool, Funcs funcs,
		           vk::ArrayProxy<const vk::PushConstantRange> PushConstantRange,
		           const Ts &... objs)
				: device{dev}, pool{desPool},
				  descriptorSetLayouts{createDescriptorSetLayouts(objs...)},
				  pipelineLayout{createPipelineLayout(PushConstantRange)}//,
				  //pipeline{func(pipelineLayout.get())}
				  {
			createDescriptorSets();
			auto pipelineiter = pipeline.begin();
			for(auto& func:funcs){
				*pipelineiter++ = func(pipelineLayout.get());
			}
		}

		gpuProgram(gpuProgram &&) = default;
	};
}
#endif //STBOX_PIPELINERESOURCE_HH
