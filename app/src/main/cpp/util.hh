//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_UTIL_H
#define STBOX_UTIL_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unistd.h>
#include <vulkan.hpp>
#include <thread>
#include <queue>
#include <condition_variable>
#include <android_native_app_glue.h>
#include <iostream>
#include <memory>
#include <map>
#include <gli/texture2d.hpp>
#include "main.hh"

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

#define SWAPCHAIN_NUM 2

namespace tt {

	class Device;

	class Window;

	class Job;

	class Instance;

	std::vector<char> loadDataFromAssets(const std::string &filePath, android_app *androidAppCtx);

	inline uint32_t
	queueFamilyPropertiesFindFlags(vk::PhysicalDevice phyDevice, vk::QueueFlags flags,
	                               vk::SurfaceKHR surface) {
		auto queueFamilyProperties = phyDevice.getQueueFamilyProperties();
		//MY_LOG(INFO) << "getQueueFamilyProperties size : "
		//          << queueFamilyProperties.size() ;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
			//MY_LOG(INFO) << "QueueFamilyProperties : " << i << "\tflags:"
			//          << vk::to_string(queueFamilyProperties[i].queueFlags) ;
			if (phyDevice.getSurfaceSupportKHR(i, surface) &&
			    (queueFamilyProperties[i].queueFlags & flags)) {
				MY_LOG(INFO) << "default_queue_index :" << i << "\tgetSurfaceSupportKHR:true";
				return i;
			}
		}
		throw std::logic_error{"queueFamilyPropertiesFindFlags Error"};
	}

	using AAssetHander = std::unique_ptr<AAsset, std::function<void(AAsset *)>>;
	inline auto AAssetManagerFileOpen(AAssetManager* assetManager,const std::string &filePath){
		return AAssetHander {
				AAssetManager_open(assetManager, filePath.c_str(),
				                   AASSET_MODE_STREAMING),
				[](AAsset *AAsset) {
					AAsset_close(AAsset);
				}
		};
	}

	struct RenderpassBeginHandle : public vk::CommandBuffer {
		RenderpassBeginHandle(vk::CommandBuffer commandBuffer,
		                      vk::RenderPassBeginInfo renderPassBeginInfo)
				: vk::CommandBuffer{commandBuffer} {
			beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		}

		~RenderpassBeginHandle() {
			endRenderPass();
		}

		RenderpassBeginHandle(const RenderpassBeginHandle &) = delete; // non construction-copyable
		RenderpassBeginHandle &operator=(const RenderpassBeginHandle &) = delete; // non copyable
	};

	struct CommandBufferBeginHandle : public vk::CommandBuffer {
		CommandBufferBeginHandle(
				vk::UniqueCommandBuffer &uniqueCommandBuffer,vk::CommandBufferUsageFlags commandBufferUsageFlags = vk::CommandBufferUsageFlagBits {}) :
				vk::CommandBuffer{uniqueCommandBuffer.get()} {
			begin(vk::CommandBufferBeginInfo{commandBufferUsageFlags});
		}

		~CommandBufferBeginHandle() {
			end();
		}

		CommandBufferBeginHandle(
				const CommandBufferBeginHandle &) = delete; // non construction-copyable
		CommandBufferBeginHandle &
		operator=(const CommandBufferBeginHandle &) = delete; // non copyable
	};

	uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice, uint32_t memoryTypeBits,
	                             vk::MemoryPropertyFlags flags);


	using ImageViewMemory = std::tuple<vk::UniqueImage, vk::UniqueImageView, vk::UniqueDeviceMemory>;

	//using ImageViewSamplerMemory = std::tuple<vk::UniqueImage, vk::UniqueImageView, vk::UniqueSampler, vk::UniqueDeviceMemory>;

	using BufferMemory = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, size_t>;

	class BufferMemoryPtr : public std::unique_ptr<void, std::function<void(void *)> > {
	public:
		using std::unique_ptr<void, std::function<void(void *)> >::unique_ptr;
		template<typename PodType>
		PodType& PodTypeOnMemory(){
			return *static_cast<PodType *>(get());
		}

	};
	namespace helper {
		template<typename Tuple>
		auto mapMemoryAndSize(vk::Device device, Tuple &tupleMemoryAndSize, size_t offset = 0) {
			return BufferMemoryPtr{
					device.mapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
					                 offset,
					                 std::get<size_t>(tupleMemoryAndSize),
					                 vk::MemoryMapFlagBits()),
					[device, &tupleMemoryAndSize](void *pVoid) {
						device.unmapMemory(
								std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get());
					}
			};
		}

		std::vector<vk::UniqueCommandBuffer>
		createCmdBuffers(vk::Device device, vk::RenderPass renderPass,
		                 tt::Job &job,
		                 std::vector<vk::UniqueFramebuffer> &framebuffers, vk::Extent2D extent2D,
		                 vk::CommandPool pool,
		                 std::function<void(Job&,RenderpassBeginHandle &, vk::Extent2D)> functionRenderpassBegin,
		                 std::function<void(Job&,CommandBufferBeginHandle &, vk::Extent2D)> functionBegin);
	}
};


class Camera {
private:
	float fov;
	float znear, zfar;

	void updateViewMatrix() {
		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM;

		rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		transM = glm::translate(glm::mat4(1.0f), position);

		if (type == CameraType::firstperson) {
			matrices.view = rotM * transM;
		} else {
			matrices.view = transM * rotM;
		}

		updated = true;
	};
public:
	enum CameraType {
		lookat, firstperson
	};
	CameraType type = CameraType::lookat;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 position = glm::vec3();

	float rotationSpeed = 1.0f;
	float movementSpeed = 1.0f;

	bool updated = false;

	struct {
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	struct {
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	bool moving() {
		return keys.left || keys.right || keys.up || keys.down;
	}

	float getNearClip() {
		return znear;
	}

	float getFarClip() {
		return zfar;
	}

	void setPerspective(float fov, float aspect, float znear, float zfar) {
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	};

	void updateAspectRatio(float aspect) {
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	}

	void setPosition(glm::vec3 position) {
		this->position = position;
		updateViewMatrix();
	}

	void setRotation(glm::vec3 rotation) {
		this->rotation = rotation;
		updateViewMatrix();
	};

	void rotate(glm::vec3 delta) {
		this->rotation += delta;
		updateViewMatrix();
	}

	void setTranslation(glm::vec3 translation) {
		this->position = translation;
		updateViewMatrix();
	};

	void translate(glm::vec3 delta) {
		this->position += delta;
		updateViewMatrix();
	}

	void update(float deltaTime) {
		updated = false;
		if (type == CameraType::firstperson) {
			if (moving()) {
				glm::vec3 camFront;
				camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
				camFront.y = sin(glm::radians(rotation.x));
				camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
				camFront = glm::normalize(camFront);

				float moveSpeed = deltaTime * movementSpeed;

				if (keys.up)
					position += camFront * moveSpeed;
				if (keys.down)
					position -= camFront * moveSpeed;
				if (keys.left)
					position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) *
					            moveSpeed;
				if (keys.right)
					position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) *
					            moveSpeed;

				updateViewMatrix();
			}
		}
	};

	// Update camera passing separate axis data (gamepad)
	// Returns true if view or position has been changed
	bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime) {
		bool retVal = false;

		if (type == CameraType::firstperson) {
			// Use the common console thumbstick layout
			// Left = view, right = move

			const float deadZone = 0.0015f;
			const float range = 1.0f - deadZone;

			glm::vec3 camFront;
			camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			camFront.y = sin(glm::radians(rotation.x));
			camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			camFront = glm::normalize(camFront);

			float moveSpeed = deltaTime * movementSpeed * 2.0f;
			float rotSpeed = deltaTime * rotationSpeed * 50.0f;

			// Move
			if (fabsf(axisLeft.y) > deadZone) {
				float pos = (fabsf(axisLeft.y) - deadZone) / range;
				position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}
			if (fabsf(axisLeft.x) > deadZone) {
				float pos = (fabsf(axisLeft.x) - deadZone) / range;
				position +=
						glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos *
						((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}

			// Rotate
			if (fabsf(axisRight.x) > deadZone) {
				float pos = (fabsf(axisRight.x) - deadZone) / range;
				rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
			if (fabsf(axisRight.y) > deadZone) {
				float pos = (fabsf(axisRight.y) - deadZone) / range;
				rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
		} else {
			// todo: move code from example base class for look-at
		}

		if (retVal) {
			updateViewMatrix();
		}

		return retVal;
	}

};

#endif //STBOX_UTIL_H
