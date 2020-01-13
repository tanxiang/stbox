//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_DEVICE_HH
#define STBOX_DEVICE_HH

#include "util.hh"
#include "Window.hh"
#include "JobBase.hh"
#include "JobDrawLine.hh"
#include "JobDraw.hh"
#include <type_traits>


namespace tt {

	template<typename T, typename _ = void>
	struct is_container : std::false_type {
	};

	template<typename... Ts>
	struct is_container_helper {
	};

	template<typename T>
	struct is_container<
			T,
			std::conditional_t<
					false,
					is_container_helper<
							typename T::value_type,
							typename T::size_type,
							typename T::allocator_type,
							typename T::iterator,
							typename T::const_iterator,
							decltype(std::declval<T>().size()),
							decltype(std::declval<T>().begin()),
							decltype(std::declval<T>().end()),
							decltype(std::declval<T>().data())
					>,
					void
			>
	> : public std::true_type {
	};

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

		vk::UniqueDevice initHander(vk::PhysicalDevice &phy, vk::SurfaceKHR &surface);

	public:
		//Device(){}

		auto createJobBase(std::vector<vk::DescriptorPoolSize> descriptorPoolSizes, size_t maxSet) {
			return tt::JobBase{get(), gQueueFamilyIndex, descriptorPoolSizes, maxSet};
		}

		Device(vk::PhysicalDevice &phy, vk::SurfaceKHR &surface, android_app *app) :
				vk::UniqueDevice{initHander(phy, surface)}, physicalDevice{phy},
				//gQueueFamilyIndex{deviceCreateInfo.pQueueCreateInfos->queueFamilyIndex},
				renderPassFormat{physicalDevice.getSurfaceFormatsKHR(surface)[0].format},
				renderPass{createRenderpass(renderPassFormat)},
				Jobs{std::make_tuple(
						createJobBase(
								{
										vk::DescriptorPoolSize{
												vk::DescriptorType::eUniformBuffer, 1
										},
										vk::DescriptorPoolSize{
												vk::DescriptorType::eStorageBuffer, 3
										}
								},
								2
						),
						app,
						this),
				     JobDraw::create(app, *this)} {

		}

/*
		Device(vk::DeviceCreateInfo deviceCreateInfo, vk::PhysicalDevice &phy,
		       vk::SurfaceKHR &surface, android_app *app) :
				vk::UniqueDevice{phy.createDeviceUnique(deviceCreateInfo)}, physicalDevice{phy},
				gQueueFamilyIndex{deviceCreateInfo.pQueueCreateInfos->queueFamilyIndex},
				renderPassFormat{physicalDevice.getSurfaceFormatsKHR(surface)[0].format},
				renderPass{createRenderpass(renderPassFormat)},
				Jobs{JobDrawLine::create(app, *this), JobDraw::create(app, *this)} {

		}
*/

		auto phyDevice() {
			return physicalDevice;
		}

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

		template<typename Tuple>
		auto mapMemoryAndSize(Tuple &tupleMemoryAndSize, size_t offset = 0) {
			auto dev = get();
			auto devMemory = std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get();
			return BufferMemoryPtr{
					get().mapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
					                offset,
					                std::get<size_t>(tupleMemoryAndSize),
					                vk::MemoryMapFlagBits()),
					[dev, devMemory](void *pVoid) {
						dev.unmapMemory(devMemory);
					}
			};
		}

		auto mapMemorySize(vk::DeviceMemory devMemory, size_t size, size_t offset = 0) {
			auto dev = get();
			return BufferMemoryPtr{
					dev.mapMemory(devMemory, offset, size,
					              vk::MemoryMapFlagBits()),
					[dev, devMemory](void *pVoid) {
						dev.unmapMemory(devMemory);
					}
			};
		}

		ImageViewMemory createImageAndMemoryFromMemory(gli::texture2d t2d,
		                                               vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

		ImageViewMemory createImageAndMemoryFromT2d(gli::texture2d t2d,
		                                            vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);


		BufferMemory
		createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
		                      vk::MemoryPropertyFlags memoryPropertyFlags);


		BufferMemory
		createBufferAndMemoryFromAssets(android_app *androidAppCtx, std::vector<std::string> names,
		                                vk::BufferUsageFlags bufferUsageFlags,
		                                vk::MemoryPropertyFlags memoryPropertyFlags);

		auto alignment(uint32_t alignment, uint32_t length) {
			return length + (alignment - 1) - ((length - 1) % alignment);
		}


		template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
		auto objSize(uint32_t alig, const T &t) {
			return t;
		}

		template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
		auto objSize(uint32_t alig, const T &t) {
			return t.size() * sizeof(typename T::value_type);
		}

		template<typename T, typename std::enable_if<
				!is_container<T>::value && !std::is_integral<T>::value, int>::type = 0>
		auto objSize(uint32_t alig, const T &t) {
			return sizeof(t);
		}

		template<typename T, typename ... Ts>
		auto objSize(uint32_t alig, const T &t, const Ts &... ts) {
			return alignment(alig, objSize(alig, t)) + objSize(alig, ts...);
		}

		template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
		vk::DescriptorBufferInfo
		writeObj(BufferMemoryPtr &ptr, vk::Buffer buffer, uint32_t alig, uint32_t boff,
		         uint32_t &off,
		         const T &t) {
			auto size = t;
			//memcpy(static_cast<char *>(ptr.get()) + off, t.data(), size);
			auto m_off = off;
			//off += get().getBufferMemoryRequirements(buffer).size;
			off += alignment(alig, objSize(alig, t));
			return vk::DescriptorBufferInfo{buffer, m_off - boff, size};
		}

		template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
		vk::DescriptorBufferInfo
		writeObj(BufferMemoryPtr &ptr, vk::Buffer buffer, uint32_t alig, uint32_t boff,
		         uint32_t &off,
		         const T &t) {
			auto size = t.size() * sizeof(typename T::value_type);
			memcpy(static_cast<char *>(ptr.get()) + off, t.data(), size);
			auto m_off = off;
			//off += get().getBufferMemoryRequirements(buffer).size;
			off += alignment(alig, objSize(alig, t));
			return vk::DescriptorBufferInfo{buffer, m_off - boff, size};
		}

		template<typename T, typename std::enable_if<
				!is_container<T>::value && !std::is_integral<T>::value, int>::type = 0>
		vk::DescriptorBufferInfo
		writeObj(BufferMemoryPtr &ptr, vk::Buffer buffer, uint32_t alig, uint32_t boff,
		         uint32_t &off,
		         const T &t) {
			auto size = sizeof(t);
			memcpy(static_cast<char *>(ptr.get()) + off, &t, size);
			auto m_off = off;
			//off += get().getBufferMemoryRequirements(buffer).size;
			off += alignment(alig, objSize(alig, t));
			return vk::DescriptorBufferInfo{buffer, m_off - boff, size};
		}

		template<typename TP, typename ... Ts>
		auto writeObjsDescriptorBufferInfo(BufferMemoryPtr &bufferPtr, TP &tp, uint32_t &off,
		                                   const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto m_off = off;
			tp.off() = off;
			tp.descriptors() = std::vector{
					writeObj(bufferPtr, tp.buffer().get(), alig, off, m_off, objs)...};
			return get().getBufferMemoryRequirements(tp.buffer().get()).size;
		}

		BufferMemory flushBufferToDevMemory(vk::BufferUsageFlags bufferUsageFlags,
		                                    vk::MemoryPropertyFlags memoryPropertyFlags,
		                                    size_t size, BufferMemory &&bufferMemory);


		void flushBufferToBuffer(vk::Buffer srcbuffer, vk::Buffer decbuffer, size_t size,
		                         size_t srcoff = 0, size_t decoff = 0);

		void flushBufferToMemory(vk::Buffer buffer, vk::DeviceMemory memory, size_t size,
		                         size_t srcoff = 0, size_t decoff = 0);

		void bindBsm(BuffersMemory<> &BsM);


		template<typename ... Ts>
		auto &buildBufferOnBsM(BuffersMemory<> &BsM, vk::BufferUsageFlags bufferUsageFlags,
		                       const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto size = objSize(alig, objs...);
			//MY_LOG(INFO) << "buffersize:" <<size;
			return BsM.desAndBuffers().emplace_back(
					get().createBufferUnique(
							vk::BufferCreateInfo{
									vk::BufferCreateFlags(),
									size,
									bufferUsageFlags}
					)
			);
		}

		auto createLocalBufferMemoryOnBsM(BuffersMemory<> &BsM) {

			size_t size = 0;
			for (auto &buffer:BsM.desAndBuffers()) {
				auto memoryRequirements = get().getBufferMemoryRequirements(buffer.buffer().get());
				size += memoryRequirements.size;
			}
			//MY_LOG(INFO) << "need locale mem :" << size;
			return createBufferAndMemory(size,
			                             vk::BufferUsageFlagBits::eTransferSrc,
			                             vk::MemoryPropertyFlagBits::eHostVisible |
			                             vk::MemoryPropertyFlagBits::eHostCoherent);

		}


		void buildMemoryOnBsM(BuffersMemory<> &BsM,
		                      vk::MemoryPropertyFlags memoryPropertyFlags) {
			size_t size = 0;
			uint32_t memoryTypeBits = 0xFFFFFFFF;
			for (auto &buffer:BsM.desAndBuffers()) {
				auto memoryRequirements = get().getBufferMemoryRequirements(buffer.buffer().get());
				memoryTypeBits &= memoryRequirements.memoryTypeBits;
				size += memoryRequirements.size;
				//MY_LOG(INFO) << size << '=' << buffer.size() << '?'
				//             << std::bitset<sizeof(memoryTypeBits) << 3>(memoryTypeBits);
			}
			auto typeIndex = findMemoryTypeIndex(memoryTypeBits, memoryPropertyFlags);
			MY_LOG(INFO) << "alloc dev mem:" << size << " index:" << typeIndex;
			BsM.memory() = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
					size, typeIndex
			});
			BsM.size() = size;
			bindBsm(BsM);
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

/*		std::map<std::string, vk::UniquePipeline> createComputePipeline(android_app *app);
*/

		vk::UniquePipeline createGraphsPipeline(
				vk::ArrayProxy<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos,
				vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo,
				vk::PipelineLayout pipelineLayout,
				vk::PipelineCache pipelineCache,
				vk::RenderPass jobRenderPass,
				vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleStrip
		);

		std::vector<vk::UniqueCommandBuffer>
		createCmdBuffers(
				size_t cmdNum, vk::CommandPool pool,
				std::function<void(CommandBufferBeginHandle &)> = [](CommandBufferBeginHandle &) {},
				vk::CommandBufferUsageFlags commandBufferUsageFlags = vk::CommandBufferUsageFlagBits{}
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


		//std::vector<JobDraw> drawJobs;
	private:
		std::tuple<JobDrawLine, JobDraw> Jobs;
	public:
		template<typename JobType>
		auto &Job(int index = 0) {
			return std::get<JobType>(Jobs);
		}
	};

}


#endif //STBOX_DEVICE_HH
