//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_DEVICE_HH
#define STBOX_DEVICE_HH

#include "util.hh"
#include "Window.hh"
namespace tt {
	class Device : public vk::UniqueDevice {
		vk::PhysicalDevice physicalDevice;
		uint32_t gQueueFamilyIndex;
		vk::UniqueCommandPool gPoolUnique = get().createCommandPoolUnique(
				vk::CommandPoolCreateInfo{
						vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
						gQueueFamilyIndex
				}
		);
		vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
		vk::Format renderPassFormat;
	public:
		vk::UniqueRenderPass renderPass;

		vk::UniqueShaderModule loadShaderFromAssets(const std::string &filePath,
		                                            android_app *androidAppCtx);

	private:

		auto findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags) {
			return tt::findMemoryTypeIndex(physicalDevice, memoryTypeBits, flags);
		}


		std::vector<std::tuple<std::string, vk::UniqueShaderModule>>
		loadShaderFromAssetsDir(const char *dirPath,
		                        android_app *androidAppCtx);

	public:
		//Device(){}

		Device(vk::DeviceCreateInfo deviceCreateInfo, vk::PhysicalDevice &phy) :
				vk::UniqueDevice{phy.createDeviceUnique(deviceCreateInfo)}, physicalDevice{phy},
				gQueueFamilyIndex{deviceCreateInfo.pQueueCreateInfos->queueFamilyIndex} {

		}

		auto phyDevice() {
			return physicalDevice;
		}

		JobBase createJob(std::vector<vk::DescriptorPoolSize> descriptorPoolSizes,
		               std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings);

		auto transQueue() {
			return get().getQueue(gQueueFamilyIndex, 0);
		}

		auto compQueue() {
			return get().getQueue(gQueueFamilyIndex, 0);
		}

		auto graphsQueue() {
			return get().getQueue(gQueueFamilyIndex, 0);
		}

		auto getDepthFormat() {
			return depthFormat;
		}

		auto getRenderPassFormat() {
			return renderPassFormat;
		}

		vk::UniqueDescriptorSetLayout
		createDescriptorSetLayoutUnique(std::vector<vk::DescriptorSetLayoutBinding> descriptSlBs) {
			return get().createDescriptorSetLayoutUnique(
					vk::DescriptorSetLayoutCreateInfo{
							vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(),
							descriptSlBs.data()
					});
		}

		vk::UniquePipelineLayout
		createPipelineLayout(vk::UniqueDescriptorSetLayout &descriptorSetLayout) {
			return get().createPipelineLayoutUnique(
					vk::PipelineLayoutCreateInfo{
							vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0,
							nullptr
					});
		}

		vk::UniqueRenderPass createRenderpass(vk::Format surfaceDefaultFormat);


		bool checkSurfaceSupport(vk::SurfaceKHR &surface) {
			auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(physicalDevice,
			                                                         vk::QueueFlagBits::eGraphics,
			                                                         surface);
			if (graphicsQueueIndex == gQueueFamilyIndex)
				return true;
			return false;
		}

		//vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

		ImageViewMemory createImageAndMemory(
				vk::Format format, vk::Extent3D extent3D,
				vk::ImageUsageFlags imageUsageFlags =
				vk::ImageUsageFlagBits::eDepthStencilAttachment |
				vk::ImageUsageFlagBits::eTransientAttachment,
				uint32_t mipLevels = 1,
				vk::ComponentMapping componentMapping = vk::ComponentMapping{},
				vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange{
						vk::ImageAspectFlagBits::eDepth |
						vk::ImageAspectFlagBits::eStencil,
						0, 1, 0, 1
				}
		);

		BufferMemory
		createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
		                      vk::MemoryPropertyFlags memoryPropertyFlags);


		BufferMemory
		createBufferAndMemoryFromAssets(android_app *androidAppCtx,std::vector<std::string> names, vk::BufferUsageFlags bufferUsageFlags,
		                      vk::MemoryPropertyFlags memoryPropertyFlags);


		template<typename Tuple>
		auto mapMemoryAndSize(Tuple &tupleMemoryAndSize, size_t offset = 0) {
			return BufferMemoryPtr{
					get().mapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
					                offset,
					                std::get<size_t>(tupleMemoryAndSize),
					                vk::MemoryMapFlagBits()),
					[this, &tupleMemoryAndSize](void *pVoid) {
						get().unmapMemory(
								std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get());
					}
			};
		}

		ImageViewMemory createImageAndMemoryFromMemory(gli::texture2d t2d,
		                                            vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);
		ImageViewMemory createImageAndMemoryFromT2d(gli::texture2d t2d,
		                                            vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

		template<typename VectorType>
		BufferMemory
		createBufferAndMemoryFromVector(VectorType data, vk::BufferUsageFlags bufferUsageFlags,
		                                vk::MemoryPropertyFlags memoryPropertyFlags) {
			auto BufferMemoryToWirte = createBufferAndMemory(
					sizeof(typename VectorType::value_type) * data.size(), bufferUsageFlags,
					memoryPropertyFlags);

			auto dataPtr = mapMemoryAndSize(BufferMemoryToWirte);
			std::memcpy(dataPtr.get(), data.data(), std::get<size_t>(BufferMemoryToWirte));
			return BufferMemoryToWirte;
		}

		template<typename MatType>
		BufferMemory
		createBufferAndMemoryFromMat(MatType data, vk::BufferUsageFlags bufferUsageFlags,
		                             vk::MemoryPropertyFlags memoryPropertyFlags) {
			auto BufferMemoryToWirte = createBufferAndMemory(sizeof(MatType), bufferUsageFlags,
			                                                 memoryPropertyFlags);

			auto dataPtr = mapMemoryAndSize(BufferMemoryToWirte);
			memcpy(dataPtr.get(), &data, std::get<size_t>(BufferMemoryToWirte));
			return BufferMemoryToWirte;
		}

		auto createSampler(uint32_t levels) {
			return get().createSamplerUnique(
					vk::SamplerCreateInfo{
							vk::SamplerCreateFlags(),
							vk::Filter::eLinear,
							vk::Filter::eLinear,
							vk::SamplerMipmapMode::eLinear,
							vk::SamplerAddressMode::eRepeat,
							vk::SamplerAddressMode::eRepeat,
							vk::SamplerAddressMode::eRepeat,
							0,
							0,
							0,
							0, vk::CompareOp::eNever, 0,
							levels,
							vk::BorderColor::eFloatOpaqueWhite,
							0});
		}

		template<typename Tuple>
		auto getDescriptorBufferInfo(Tuple &tupleMemoryAndSize, vk::DeviceSize size = VK_WHOLE_SIZE,
		                             vk::DeviceSize offset = 0) {
			return vk::DescriptorBufferInfo{std::get<vk::UniqueBuffer>(tupleMemoryAndSize).get(),
			                                offset, size};
		}

		template<typename Tuple>
		auto getDescriptorImageInfo(Tuple &tupleImage, vk::Sampler &sampler) {
			return vk::DescriptorImageInfo{
					sampler, std::get<vk::UniqueImageView>(tupleImage).get(),
					vk::ImageLayout::eShaderReadOnlyOptimal
			};
		}

		std::map<std::string, vk::UniquePipeline> createComputePipeline(android_app *app);

		vk::UniquePipeline createPipeline(uint32_t dataStepSize,
		                                  std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos,
		                                  vk::PipelineCache &pipelineCache,
		                                  vk::PipelineLayout pipelineLayout);


		std::vector<vk::UniqueCommandBuffer>
		createCmdBuffers(
				size_t cmdNum, vk::CommandPool pool,
				std::function<void(CommandBufferBeginHandle &)> = [](CommandBufferBeginHandle &) {},
				vk::CommandBufferUsageFlags commandBufferUsageFlags = vk::CommandBufferUsageFlagBits {}
		);

		std::vector<vk::UniqueCommandBuffer>
		createCmdBuffers(
				tt::Window &swapchain, vk::CommandPool pool,
				std::function<void(RenderpassBeginHandle &)> = [](RenderpassBeginHandle &) {},
				std::function<void(CommandBufferBeginHandle &)> = [](CommandBufferBeginHandle &) {}
		);


		vk::UniqueFence submitCmdBuffer(vk::CommandBuffer &commandBuffer) {
			auto fence = get().createFenceUnique(vk::FenceCreateInfo{});
			std::array submitInfos{
					vk::SubmitInfo{
							0, nullptr, nullptr,
							1, &commandBuffer
					}
			};
			graphsQueue().submit(submitInfos, fence.get());
			return fence;
		}

		auto waitFence(vk::Fence &Fence) {
			return get().waitForFences(1, &Fence, true, 10000000);
		}

		template<typename Tjob>
		void runJobOnWindow(Tjob &j, tt::Window &win) {
			auto imageAcquiredSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			auto renderSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			auto renderFence = win.submitCmdBuffer(*this, j.cmdBuffers,
			                                       imageAcquiredSemaphore.get(),
			                                       renderSemaphore.get());
			waitFence(renderFence.get());
		}
	};

}


#endif //STBOX_DEVICE_HH
