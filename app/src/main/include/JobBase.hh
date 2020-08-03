//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_JOBBASE_HH
#define STBOX_JOBBASE_HH

#include "util.hh"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_
#define GLM_HAS_IF_CONSTEXPR
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace tt {

	struct JobBase {
	protected:
		static glm::mat4 perspective;
		static glm::mat4 lookat;
		static glm::qua<float> fRotate;
		vk::UniqueDescriptorPool descriptorPool;
		auto ownerDevice() {
			return descriptorPool.getOwner();
		}

		vk::UniquePipelineCache pipelineCache;

		vk::UniqueCommandPool commandPool;
	public:
		JobBase(vk::Device device, uint32_t queueIndex,
		        vk::ArrayProxy<vk::DescriptorPoolSize> descriptorPoolSizes, size_t maxSet) :
				descriptorPool{
						device.createDescriptorPoolUnique(
								vk::DescriptorPoolCreateInfo{
										vk::DescriptorPoolCreateFlags(),
										maxSet,
										descriptorPoolSizes.size(),
										descriptorPoolSizes.data()
								}
						)
				},
				commandPool{
						ownerDevice().createCommandPoolUnique(vk::CommandPoolCreateInfo{
								vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex}
						)
				}{}

		static auto& getPerspective(){
			return perspective;
		}

		static void setRotate(float dx = 0.0, float dy = 0.0);

	};
}

#endif //STBOX_JOBBASE_HH
