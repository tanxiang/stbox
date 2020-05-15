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
#include "JobSkyBox.hh"
#include "JobIsland.hh"

#include "ktx2.hh"
#include <type_traits>
//#include <gli/texture_cube.hpp>


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


	template<typename T, typename _ = void>
	struct is_io_obj : std::false_type {
	};

	template<typename... Ts>
	struct is_io_obj_helper {
	};

	template<typename T>
	struct is_io_obj<
			T,
			std::conditional_t<
					false,
					is_io_obj_helper<
							decltype(std::declval<T>().getLength()),
							decltype(std::declval<T>().read(nullptr, 0))
					>,
					void
			>
	> : public std::true_type {
	};

	auto inline alignment(uint32_t alignment, uint32_t length) {
		return length + (alignment - 1) - ((length - 1) % alignment);
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
	auto objSize(uint32_t alig, const T &t) {
		return alignment(alig, t);
	}

	template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
	auto objSize(uint32_t alig, const T &t) {
		return alignment(alig, t.size() * sizeof(typename T::value_type));
	}


	template<typename T, typename std::enable_if<is_io_obj<T>::value, int>::type = 0>
	auto objSize(uint32_t alig, const T &t) {
		return alignment(alig, t.getLength());
	}

	template<typename T, typename std::enable_if<
			!is_io_obj<T>::value && !is_container<T>::value &&
			!std::is_integral<T>::value, int>::type = 0>
	auto objSize(uint32_t alig, const T &t) {
		return alignment(alig, sizeof(t));
	}

	template<typename T, typename ... Ts>
	auto objSize(uint32_t alig, const T &t, const Ts &... ts) {
		return objSize(alig, t) + objSize(alig, ts...);
	}

	template<typename T>
	auto objSizeOffset(uint32_t alig, uint32_t &offset, const T &t) {
		offset += objSize(alig, t);
		return offset;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
	auto objDataSize(uint32_t alig, const T &t) {
		return 0;
	}

	template<typename T, typename std::enable_if<is_container<T>::value, int>::type = 0>
	auto objDataSize(uint32_t alig, const T &t) {
		return alignment(alig, t.size() * sizeof(typename T::value_type));
	}

	template<typename T, typename std::enable_if<is_io_obj<T>::value, int>::type = 0>
	auto objDataSize(uint32_t alig, const T &t) {
		return alignment(alig, t.getLength());
	}

	template<typename T, typename std::enable_if<
			!is_io_obj<T>::value && !is_container<T>::value &&
			!std::is_integral<T>::value, int>::type = 0>
	auto objDataSize(uint32_t alig, const T &t) {
		return alignment(alig, sizeof(t));
	}

	template<typename T, typename ... Ts>
	auto objDataSize(uint32_t alig, const T &t, const Ts &... ts) {
		return objDataSize(alig, t) + objDataSize(alig, ts...);
	}

	template<typename T>
	auto objDataSizeOffset(uint32_t alig, uint32_t &offset, const T &t) {
		offset += objDataSize(alig, t);
		return offset;
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
		off += objSize(alig, t);
		return vk::DescriptorBufferInfo{buffer, m_off - boff, size};
	}

	template<typename T, typename std::enable_if<is_io_obj<T>::value, int>::type = 0>
	vk::DescriptorBufferInfo
	writeObj(BufferMemoryPtr &ptr, vk::Buffer buffer, uint32_t alig, uint32_t boff,
	         uint32_t &off,
	         const T &t) {
		t.read(static_cast<char *>(ptr.get()) + off, t.getLength());
		uint16_t * pp = reinterpret_cast<uint16_t *> (static_cast<char *>(ptr.get()) + off);
		//MY_LOG(INFO) <<off <<": "<< pp[0] << ' ' << pp[1] << ' ' << pp[2] << ' ' << pp[3] << ' ';
		auto m_off = off;
		//off += get().getBufferMemoryRequirements(buffer).size;
		off += objSize(alig, t);
		return vk::DescriptorBufferInfo{buffer, m_off - boff, t.getLength()};
	}

	template<typename T, typename std::enable_if<
			!is_io_obj<T>::value && !is_container<T>::value &&
			!std::is_integral<T>::value, int>::type = 0>
	vk::DescriptorBufferInfo
	writeObj(BufferMemoryPtr &ptr, vk::Buffer buffer, uint32_t alig, uint32_t boff,
	         uint32_t &off,
	         const T &t) {
		auto size = sizeof(t);
		memcpy(static_cast<char *>(ptr.get()) + off, &t, size);
		auto m_off = off;
		//off += get().getBufferMemoryRequirements(buffer).size;
		off += objSize(alig, t);
		return vk::DescriptorBufferInfo{buffer, m_off - boff, size};
	}


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
		vk::UniqueCommandPool commandPool;

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
				commandPool{
						get().createCommandPoolUnique(vk::CommandPoolCreateInfo{
								vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
								gQueueFamilyIndex}
						)
				},
				Jobs{
						std::make_tuple(
								app,
								this
						),
						std::make_tuple(
								app,
								this
						),
						JobDraw::create(app, *this),
						std::make_tuple(
								app,
								this
						),
				} {

		}


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


		template<typename ... Ts>
		auto writeObjsToPtr(BufferMemoryPtr &bufferPtr, uint32_t &off,
		                    const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto m_off = off;
			return std::array{
					writeObj(bufferPtr, {}, alig, off, m_off, objs)...};
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

		template<typename TupleType>
		auto mapBufferMemory(const TupleType &tuple, size_t offset = 0) {
			return mapMemorySize(
					std::get<vk::UniqueDeviceMemory>(tuple).get(),
					get().getBufferMemoryRequirements(
							std::get<vk::UniqueBuffer>(tuple).get()).size - offset, offset);
		}

		template<typename PodType, typename TupleType>
		auto mapTypeBufferMemory(const TupleType &tuple, size_t offset = 0) {
			auto devMemory = std::get<vk::UniqueDeviceMemory>(tuple).get();
			auto device = get();
			return BufferTypePtr<PodType>{
					static_cast<PodType *>(device.mapMemory(
							std::get<vk::UniqueDeviceMemory>(tuple).get(),
							offset,
							get().getBufferMemoryRequirements(
									std::get<vk::UniqueBuffer>(tuple).get()).size - offset,
							vk::MemoryMapFlagBits())),
					[device, devMemory](PodType *pVoid) {
						//FIXME call ~PodType in array
						device.unmapMemory(devMemory);
					}
			};
		}

		//ImageViewMemory createImageAndMemoryFromMemory(gli::texture2d t2d,
		//                                               vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);

		//void writeTextureToImage(gli::texture_cube &texture, vk::Image image);

		void writeTextureToImage(ktx2 &texture, vk::Image image);

		//void writeTextureToImage(gli::texture2d &texture, vk::Image image);


		//ImageViewMemory createImageAndMemoryFromT2d(gli::texture2d t2d,
		//                                            vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled);


		BufferMemory
		createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
		                      vk::MemoryPropertyFlags memoryPropertyFlags);


		LocalBufferMemory
		createLocalBufferMemory(size_t dataSize, vk::BufferUsageFlags flags);

		BufferMemory
		createBufferAndMemoryFromAssets(android_app *androidAppCtx, std::vector<std::string> names,
		                                vk::BufferUsageFlags bufferUsageFlags,
		                                vk::MemoryPropertyFlags memoryPropertyFlags);


		BufferMemory flushBufferToDevMemory(vk::BufferUsageFlags bufferUsageFlags,
		                                    vk::MemoryPropertyFlags memoryPropertyFlags,
		                                    size_t size, BufferMemory &&bufferMemory);


		void flushBufferToBuffer(vk::Buffer srcbuffer, vk::Buffer decbuffer, size_t size,
		                         size_t srcoff = 0, size_t decoff = 0);

		//void flushBufferToMemory(vk::Buffer buffer, vk::DeviceMemory memory, size_t size,
		//                         size_t srcoff = 0, size_t decoff = 0);

		template<typename TupleFrom, typename TupleTo>
		auto flushBufferTuple(const TupleFrom &from, const TupleTo &to, size_t srcoff = 0,
		                      size_t decoff = 0) {
			flushBufferToBuffer(std::get<vk::UniqueBuffer>(from).get(),
			                    std::get<vk::UniqueBuffer>(to).get(),
			                    get().getBufferMemoryRequirements(
					                    std::get<vk::UniqueBuffer>(from).get()).size, srcoff,
			                    decoff);
		}

		void bindBsm(BuffersMemory<> &BsM);

		template<typename Tuple>
		auto bindBufferMemory(const Tuple &tuple) {
			return get().bindBufferMemory(
					std::get<vk::UniqueBuffer>(tuple).get(),
					std::get<vk::UniqueDeviceMemory>(tuple).get(), 0);
		}

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

		template<typename ... Ts>
		auto createStagingBufferMemoryOnObjs(const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto size = objSize(alig, objs...);
			return createLocalBufferMemory(size, vk::BufferUsageFlagBits::eTransferSrc);
		}


		template<typename ... Ts>
		auto createStagingBufferMemoryOnObjs2(const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto size = objDataSize(alig, objs...);
			auto tuple = createLocalBufferMemory(size, vk::BufferUsageFlagBits::eTransferSrc);
			auto memoryPtr = mapBufferMemory(tuple);
			uint32_t offset = 0;
			writeObjsToPtr(memoryPtr, offset, objs...);
			return tuple;
		}

		template<typename ... Ts>
		auto createLocalBufferMemoryOnObjs(vk::BufferUsageFlags flags, const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			auto size = objSize(alig, objs...);
			return createLocalBufferMemory(size, flags);
		}

		template<typename Tuple>
		auto bufferTupleCreateMemory(vk::MemoryPropertyFlags memoryPropertyFlags,
		                             Tuple &tuple) {
			auto memoryReq = get().getBufferMemoryRequirements(
					std::get<vk::UniqueBuffer>(tuple).get());
			std::get<vk::UniqueDeviceMemory>(tuple) =
					get().allocateMemoryUnique(vk::MemoryAllocateInfo{
							memoryReq.size,
							findMemoryTypeIndex(memoryReq.memoryTypeBits, memoryPropertyFlags)
					});
			bindBufferMemory(tuple);
		}

		template<typename ... Ts>
		auto createBufferPartsOnObjs(vk::BufferUsageFlags flags,
		                             const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			uint32_t allSize = 0;
			std::array parts{objSizeOffset(alig, allSize, objs)...};
			auto tuple = BufferMemoryWithParts<sizeof...(objs)>(
					get().createBufferUnique(
							vk::BufferCreateInfo{
									vk::BufferCreateFlags(),
									allSize,
									flags}
					),
					vk::UniqueDeviceMemory{},parts);
			bufferTupleCreateMemory(vk::MemoryPropertyFlagBits::eDeviceLocal, tuple);
			auto staging = createStagingBufferMemoryOnObjs2(objs...);
			flushBufferTuple(staging, tuple);
			return tuple;
		}

		template<typename ... Ts>
		auto createBufferPartsdOnAssertDir(vk::BufferUsageFlags flags,
		                                   AAssetManager* aAssetManager,
		                                   const std::string dirName,
		                                   const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			uint32_t allSize = 0;
			auto dir = AAssetDirHander{aAssetManager, dirName};
			std::vector<uint32_t> parts;
			while (auto name = dir.getNextFileName()) {
				AAssetHander file{aAssetManager, dirName + '/' + name};
				allSize += alignment(alig, file.getLength());
				parts.emplace_back(allSize);
				MY_LOG(INFO) << name << file.getLength();
			}
			//std::array aParts{objSizeOffset(alig, allSize, objs)...};
			MY_LOG(INFO) << "filesNum:" << parts.size();
			parts.emplace_back(objSizeOffset(alig, allSize, objs)...);
			MY_LOG(INFO) << "objNum:" << parts.size();
			auto tuple = BufferMemoryWithPartsd(
					get().createBufferUnique(
							vk::BufferCreateInfo{
									vk::BufferCreateFlags(),
									allSize,
									flags}
					),
					vk::UniqueDeviceMemory{}, parts);
			auto staging = createLocalBufferMemory(*(parts.rbegin() + sizeof...(objs)),
			                                       vk::BufferUsageFlagBits::eTransferSrc);
			//createStagingBufferMemoryOnObjs2(*(parts.rbegin()+sizeof...(objs)),objs...);
			{
				auto memoryPtr = mapTypeBufferMemory<uint8_t>(staging);

				dir.rewind();
				uint32_t fIndex=0;
				while (auto name = dir.getNextFileName()) {
					AAssetHander file{aAssetManager, dirName + '/' + name};
					file.read(&memoryPtr[ fIndex ? parts[fIndex-1] : 0],
							fIndex ? parts[fIndex]-parts[fIndex-1] : parts[fIndex]);
					++fIndex;
				}
			}
				//std::vector parts{objSizeOffset(alig, allSize, objs)...};
			flushBufferTuple(staging, tuple);
			return tuple;

		}

		template<typename Tuple>
		auto bufferImageTupleCreateMemory(vk::MemoryPropertyFlags memoryPropertyFlags,
		                                  Tuple &tuple) {
			auto memoryReq = get().getBufferMemoryRequirements(
					std::get<vk::UniqueBuffer>(tuple).get());
			auto imageMemoryReq = get().getImageMemoryRequirements(
					std::get<vk::UniqueImage>(tuple).get());
			std::get<vk::UniqueDeviceMemory>(tuple) =
					get().allocateMemoryUnique({
							memoryReq.size + imageMemoryReq.size,
							findMemoryTypeIndex(
									memoryReq.memoryTypeBits | imageMemoryReq.memoryTypeBits,
									memoryPropertyFlags)
					});
			bindBufferMemory(tuple);
			get().bindImageMemory(std::get<vk::UniqueImage>(tuple).get(),
			                      std::get<vk::UniqueDeviceMemory>(tuple).get(), memoryReq.size);
			//MY_LOG(INFO)<<"bindImageMemory off:"<<memoryReq.size;
		}

		template<typename ... Ts>
		auto createImageBufferPartsOnObjs(vk::BufferUsageFlags flags,
				                          vk::ImageCreateInfo imageCreateInfo,
				                          const Ts &... objs) {
			auto alig = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
			uint32_t allSize = 0;
			std::array parts{objSizeOffset(alig, allSize, objs)...};
			//MY_LOG(INFO) << "allSize" <<allSize;
			auto tuple = BufferImageMemoryWithParts<sizeof...(objs)>(
					get().createBufferUnique(
							vk::BufferCreateInfo{
									vk::BufferCreateFlags(),
									allSize,
									flags}
					),
					get().createImageUnique(imageCreateInfo),
					vk::UniqueDeviceMemory{}, parts);
			bufferImageTupleCreateMemory(vk::MemoryPropertyFlagBits::eDeviceLocal, tuple);
			auto staging = createStagingBufferMemoryOnObjs2(objs...);
			flushBufferTuple(staging, tuple);
			return tuple;
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
				vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleStrip,
				vk::PolygonMode polygonMode = vk::PolygonMode::eFill
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

		//template<typename Tjob>
		void runJobOnWindow(tt::Window &win) {
			auto imageAcquiredSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			auto renderSemaphore = get().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
			auto renderFence = win.submitCmdBuffer(*this, mainCmdBuffers,
			                                       imageAcquiredSemaphore.get(),
			                                       renderSemaphore.get());
			waitFence(renderFence.get());
		}


		//std::vector<JobDraw> drawJobs;
	private:
		std::tuple<JobSkyBox, JobDrawLine, JobDraw ,JobIsland> Jobs;
	public:
		std::vector<vk::UniqueCommandBuffer> mainCmdBuffers;

		template<typename JobType>
		auto &Job(int index = 0) {
			return std::get<JobType>(Jobs);
		}

		std::array<vk::ClearValue, 2> clearValues{
				vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.2f, 0.2f}},
				vk::ClearDepthStencilValue{1.0f, 0},
		};

		void CmdBufferBegin(CommandBufferBeginHandle &, vk::Extent2D, uint32_t frameIndex);

		void CmdBufferRenderpassBegin(RenderpassBeginHandle &, vk::Extent2D, uint32_t frameIndex);

		void buildCmdBuffer(tt::Window &window);
	};

}


#endif //STBOX_DEVICE_HH
