//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOBBASE_HH
#define STBOX_JOBBASE_HH

#include "util.hh"

namespace tt {

	struct JobBase {
		//tt::Device &device;
		vk::UniqueDescriptorPool descriptorPoll;

		auto ownerDevice() {
			return descriptorPoll.getOwner();
		}

		//vk::UniqueDescriptorSetLayout descriptorSetLayout;//todo vector UniqueDescriptorSetLayout
		std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts;

		std::vector<vk::UniqueDescriptorSet> descriptorSets;
		//vk::UniqueRenderPass renderPass;
		vk::UniquePipelineCache pipelineCache;
		//vk::UniquePipelineLayout pipelineLayout;//todo vector
		std::vector<vk::UniquePipelineLayout> pipelineLayouts;
		vk::UniqueCommandPool commandPool;

	private:
		auto createDescriptorSetLayouts(
				std::initializer_list<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindings) {
			std::vector<vk::UniqueDescriptorSetLayout> DSL;
			for (auto &descriptorSetLayoutBinding:descriptorSetLayoutBindings) {
				DSL.emplace_back(
						ownerDevice().createDescriptorSetLayoutUnique(
								vk::DescriptorSetLayoutCreateInfo{
										vk::DescriptorSetLayoutCreateFlags(),
										descriptorSetLayoutBinding.size(),
										descriptorSetLayoutBinding.data()
								}
						)
				);
			}
			return DSL;
		}

		auto createDescriptorSets() {
			std::vector<vk::UniqueDescriptorSet> DS;
			for (auto &descriptorSetLayout:descriptorSetLayouts) {
				auto TDS = ownerDevice().allocateDescriptorSetsUnique(
						vk::DescriptorSetAllocateInfo{
								descriptorPoll.get(), 1,//FIXME TDS num??
								descriptorSetLayout.operator->()
						}
				);
				std::move(TDS.begin(), TDS.end(), std::inserter(DS, DS.end()));
			}
			return DS;
		}

		auto createPipelineLayouts() {
			std::vector<vk::UniquePipelineLayout> PLS;
			for (auto &descriptorSetLayout:descriptorSetLayouts) {
				pipelineLayouts.emplace_back(
						ownerDevice().createPipelineLayoutUnique(
								vk::PipelineLayoutCreateInfo{
										vk::PipelineLayoutCreateFlags(), 1, //FIXME PLS num??
										descriptorSetLayout.operator->(), 0,
										nullptr
								}
						)
				);
			}
			return PLS;
		}

	public:


		//JobBase(vk::Device device, uint32_t queueIndex,
		//        vk::ArrayProxy<vk::DescriptorPoolSize> descriptorPoolSizes);
		//        std::vector<vk::DescriptorSetLayoutBinding> &&descriptorSetLayoutBindings);

		JobBase(vk::Device device, uint32_t queueIndex,
		        vk::ArrayProxy<vk::DescriptorPoolSize> descriptorPoolSizes,
		        std::initializer_list<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindings) :
				descriptorPoll{
						device.createDescriptorPoolUnique(
								vk::DescriptorPoolCreateInfo{
										vk::DescriptorPoolCreateFlags(),
										descriptorSetLayoutBindings.size(),
										descriptorPoolSizes.size(),
										descriptorPoolSizes.data()
								}
						)
				},
				descriptorSetLayouts{
						createDescriptorSetLayouts(descriptorSetLayoutBindings)
				},
				descriptorSets{
						createDescriptorSets()
				},
				pipelineCache{
						ownerDevice().createPipelineCacheUnique(vk::PipelineCacheCreateInfo{})
				},
				pipelineLayouts{
						createPipelineLayouts()
				},
				commandPool{
						ownerDevice().createCommandPoolUnique(vk::CommandPoolCreateInfo{
								vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex}
						)
				} {}

	};
}

#endif //STBOX_JOBBASE_HH
