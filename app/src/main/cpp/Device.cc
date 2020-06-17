//
// Created by ttand on 19-8-2.
//

#include "JobBase.hh"
#include "Device.hh"
#include "Window.hh"

namespace tt {
	vk::UniqueShaderModule Device::loadShaderFromAssets(const std::string &filePath,
	                                                    android_app *androidAppCtx) {
		// Read the file
		auto fileContent = loadDataFromAssets(filePath, androidAppCtx);
		return get().createShaderModuleUnique(vk::ShaderModuleCreateInfo{
				{}, fileContent.size(),
				reinterpret_cast<const uint32_t *>(fileContent.data())});

	}

	std::vector<std::tuple<std::string, vk::UniqueShaderModule>>
	Device::loadShaderFromAssetsDir(const char *dirPath, android_app *androidAppCtx) {
		assert(androidAppCtx);
		std::unique_ptr<AAssetDir, std::function<void(AAssetDir *)> > dir{
				AAssetManager_openDir(androidAppCtx->activity->assetManager, dirPath),
				[](AAssetDir *assetDir) {
					AAssetDir_close(assetDir);
				}
		};
		std::vector<std::tuple<std::string, vk::UniqueShaderModule>> ShaderModules{};
		for (const char *fileName = AAssetDir_getNextFileName(
				dir.get()); fileName; fileName = AAssetDir_getNextFileName(dir.get())) {
			MY_LOG(INFO) << fileName;
			if (std::strstr(fileName, ".comp.spv")) {
				std::string fullName{dirPath};
				ShaderModules.emplace_back(fileName, loadShaderFromAssets(fullName + '/' + fileName,
				                                                          androidAppCtx));
			}
		}
		return ShaderModules;
	}

/*
	std::map<std::string, vk::UniquePipeline> Device::createComputePipeline(android_app *app) {
		auto shaderModules = loadShaderFromAssetsDir("shaders", app);
		std::map<std::string, vk::UniquePipeline> mapComputePipeline;
		auto descriptorSetLayout = createDescriptorSetLayoutUnique(
				std::vector<vk::DescriptorSetLayoutBinding>{
						vk::DescriptorSetLayoutBinding{
								0,
								vk::DescriptorType::eStorageImage,
								1,
								vk::ShaderStageFlagBits::eCompute
						},
						vk::DescriptorSetLayoutBinding{
								1,
								vk::DescriptorType::eStorageImage,
								1,
								vk::ShaderStageFlagBits::eCompute
						}
				}
		);
		auto pipelineLayout = createPipelineLayout(descriptorSetLayout);

		for (auto &shaderModule:shaderModules) {
			vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
					vk::PipelineShaderStageCreateFlags(),
					vk::ShaderStageFlagBits::eCompute,
					std::get<vk::UniqueShaderModule>(shaderModule).get(),
					"forward"
			};
			vk::ComputePipelineCreateInfo computePipelineCreateInfo{
					vk::PipelineCreateFlags(),
					shaderStageCreateInfo,
					pipelineLayout.get()
			};
			mapComputePipeline.emplace(std::get<std::string>(shaderModule),
			                           get().createComputePipelineUnique(pipelineCache.get(),
			                                                             computePipelineCreateInfo));
		}
		return mapComputePipeline;
	};


*/
	vk::UniqueRenderPass Device::createRenderpass(vk::Format surfaceDefaultFormat) {
		//auto surfaceDefaultFormat = device.getSurfaceDefaultFormat(surface.get());
		renderPassFormat = surfaceDefaultFormat;
		std::array attachDescs{
				vk::AttachmentDescription{
						{},
						renderPassFormat,
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eClear,
						vk::AttachmentStoreOp::eStore,
						vk::AttachmentLoadOp::eDontCare,
						vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::ePresentSrcKHR
				},
				vk::AttachmentDescription{
						{},
						depthFormat,
						vk::SampleCountFlagBits::e1,
						vk::AttachmentLoadOp::eClear,
						vk::AttachmentStoreOp::eStore,
						vk::AttachmentLoadOp::eClear,
						vk::AttachmentStoreOp::eDontCare,
						vk::ImageLayout::eUndefined,
						vk::ImageLayout::eDepthStencilAttachmentOptimal
				}
		};
		std::array attachmentRefs{
				vk::AttachmentReference{
						0, vk::ImageLayout::eColorAttachmentOptimal
				}
		};
		vk::AttachmentReference depthAttacheRefs{
				1, vk::ImageLayout::eDepthStencilAttachmentOptimal
		};
		std::array subpassDescs{
				vk::SubpassDescription{
						{},
						vk::PipelineBindPoint::eGraphics,
						0, nullptr,
						attachmentRefs.size(), attachmentRefs.data(),
						nullptr,
						&depthAttacheRefs,
				}
		};
		std::array subpassDeps{
				vk::SubpassDependency{
						VK_SUBPASS_EXTERNAL, 0,
						vk::PipelineStageFlagBits::eBottomOfPipe,
						vk::PipelineStageFlagBits::eColorAttachmentOutput,
						vk::AccessFlagBits::eMemoryRead,
						vk::AccessFlagBits::eColorAttachmentRead |
						vk::AccessFlagBits::eColorAttachmentWrite,
						vk::DependencyFlagBits::eByRegion
				},
				vk::SubpassDependency{
						0, VK_SUBPASS_EXTERNAL,
						vk::PipelineStageFlagBits::eColorAttachmentOutput,
						vk::PipelineStageFlagBits::eBottomOfPipe,
						vk::AccessFlagBits::eColorAttachmentRead |
						vk::AccessFlagBits::eColorAttachmentWrite,
						vk::AccessFlagBits::eMemoryRead,
						vk::DependencyFlagBits::eByRegion
				}
		};
		return get().createRenderPassUnique(vk::RenderPassCreateInfo{
				{},
				attachDescs.size(), attachDescs.data(),
				subpassDescs.size(), subpassDescs.data(),
				subpassDeps.size(), subpassDeps.data()
		});
	}

	std::vector<vk::UniqueCommandBuffer>
	Device::createCmdBuffers(size_t cmdNum, vk::CommandPool pool,
	                         std::function<void(CommandBufferBeginHandle &)> functionBegin,
	                         vk::CommandBufferUsageFlags commandBufferUsageFlags) {
		//MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << cmdNum;
		std::vector<vk::UniqueCommandBuffer> commandBuffers = get().allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo{pool, vk::CommandBufferLevel::ePrimary, cmdNum});
		for (auto &cmdBuffer : commandBuffers) {
			CommandBufferBeginHandle cmdBeginHandle{cmdBuffer, commandBufferUsageFlags};
			functionBegin(cmdBeginHandle);
		}
		return commandBuffers;
	}

	ImageViewMemory Device::createImageAndMemory(vk::Format format, vk::Extent3D extent3D,
	                                             vk::ImageUsageFlags imageUsageFlags,
	                                             uint32_t mipLevels,
	                                             vk::ComponentMapping componentMapping,
	                                             vk::ImageSubresourceRange imageSubresourceRange) {
		ImageViewMemory IVM{};
		std::get<vk::UniqueImage>(IVM) = get().createImageUnique(
				vk::ImageCreateInfo{
						{},
						vk::ImageType::e2D,
						format,
						extent3D,
						mipLevels,
						1,
						vk::SampleCountFlagBits::e1,
						vk::ImageTiling::eOptimal,
						imageUsageFlags
				}
		);

		auto imageMemoryRq = get().getImageMemoryRequirements(std::get<vk::UniqueImage>(IVM).get());
		std::get<vk::UniqueDeviceMemory>(IVM) = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
				imageMemoryRq.size, findMemoryTypeIndex(imageMemoryRq.memoryTypeBits,
				                                        vk::MemoryPropertyFlagBits::eDeviceLocal)
		});
		get().bindImageMemory(std::get<vk::UniqueImage>(IVM).get(),
		                      std::get<vk::UniqueDeviceMemory>(IVM).get(), 0);

		std::get<vk::UniqueImageView>(IVM) = get().createImageViewUnique(
				vk::ImageViewCreateInfo{{},
				                        std::get<vk::UniqueImage>(IVM).get(),
				                        vk::ImageViewType::e2D,
				                        format,
				                        componentMapping,
				                        imageSubresourceRange});
		return IVM;
	}

	BufferMemory
	Device::createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
	                              vk::MemoryPropertyFlags memoryPropertyFlags) {

		BufferMemory BM{};
		std::get<vk::UniqueBuffer>(BM) = get().createBufferUnique(
				vk::BufferCreateInfo{
						{},
						dataSize,
						bufferUsageFlags}
		);
		auto memoryRequirements = get().getBufferMemoryRequirements(
				std::get<vk::UniqueBuffer>(BM).get());
		auto typeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
		                                     memoryPropertyFlags);
		std::get<vk::UniqueDeviceMemory>(BM) = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
				memoryRequirements.size, typeIndex
		});
		std::get<size_t>(BM) = memoryRequirements.size;
		get().bindBufferMemory(std::get<vk::UniqueBuffer>(BM).get(),
		                       std::get<vk::UniqueDeviceMemory>(BM).get(), 0);
		return BM;
	}

	LocalBufferMemory
	Device::createLocalBufferMemory(size_t dataSize, vk::BufferUsageFlags flags) {
		vk::UniqueBuffer buffer = get().createBufferUnique(
				vk::BufferCreateInfo{
						{},
						dataSize,
						flags}
		);

		auto memoryRequirements = get().getBufferMemoryRequirements(buffer.get());
		auto typeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
		                                     vk::MemoryPropertyFlagBits::eHostVisible |
		                                     vk::MemoryPropertyFlagBits::eHostCoherent);
		vk::UniqueDeviceMemory memory = get().allocateMemoryUnique(vk::MemoryAllocateInfo{
				memoryRequirements.size, typeIndex
		});

		get().bindBufferMemory(buffer.get(), memory.get(), 0);
		return LocalBufferMemory{std::move(buffer), std::move(memory)};
	}


	BufferMemory Device::createBufferAndMemoryFromAssets(android_app *androidAppCtx,
	                                                     std::vector<std::string> names,
	                                                     vk::BufferUsageFlags bufferUsageFlags,
	                                                     vk::MemoryPropertyFlags memoryPropertyFlags) {
		auto alignment = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
		off_t offset = 0;
		std::vector<std::tuple<AAssetHander, off_t, off_t >> fileHanders;
		for (auto &name:names) {
			AAssetHander file{androidAppCtx->activity->assetManager, name};
			if (!file)
				throw std::runtime_error{"asset not found"};
			auto length = file.getLength();
			fileHanders.emplace_back(std::move(file), offset, length);
			length += (alignment - 1) - ((length - 1) % alignment);
			offset += length;
		}
		if (memoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
			auto StagingBufferMemory = createStagingBufferMemoryOnObjs(offset);
			{
				auto bufferPtr = mapBufferMemory(StagingBufferMemory);
				for (auto &file:fileHanders) {
					std::get<0>(file).read(
							(char *) bufferPtr.get() + std::get<1>(file),
							std::get<2>(file));
					MY_LOG(INFO) << "off " << std::get<1>(file) << " size " << std::get<2>(file)
					             << " Align" << alignment;
				}
			}
			auto BAM2 = createBufferAndMemory(offset, bufferUsageFlags, memoryPropertyFlags);
			auto copyCmd = createCmdBuffers(
					1, gPoolUnique.get(),
					[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
						commandBufferBeginHandle.copyBuffer(
								std::get<vk::UniqueBuffer>(StagingBufferMemory).get(),
								std::get<vk::UniqueBuffer>(BAM2).get(),
								{vk::BufferCopy{0, 0, offset}});
					},
					vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

			auto copyFence = submitCmdBuffer(copyCmd[0].get());
			waitFence(copyFence.get());
			for (auto &file:fileHanders) {
				std::get<std::vector<vk::DescriptorBufferInfo>>(BAM2).emplace_back(
						std::get<vk::UniqueBuffer>(BAM2).get(), std::get<1>(file),
						std::get<2>(file));
			}
			return BAM2;
		}

		auto BAM = createBufferAndMemory(offset, bufferUsageFlags, memoryPropertyFlags);
		auto bufferPtr = mapBufferMemory(BAM);
		for (auto &file:fileHanders) {
			std::get<0>(file).read((char *) bufferPtr.get() + std::get<1>(file),
			                       std::get<2>(file));
			std::get<std::vector<vk::DescriptorBufferInfo>>(BAM).emplace_back(
					std::get<vk::UniqueBuffer>(BAM).get(), std::get<1>(file), std::get<2>(file));
			MY_LOG(INFO) << "off " << std::get<1>(file) << " size " << std::get<2>(file) << " Align"
			             << alignment;
		}
		return BAM;
	}

	vk::UniquePipeline Device::createGraphsPipeline(
			vk::ArrayProxy<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos,
			vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo,
			vk::PipelineLayout pipelineLayout,
			vk::PipelineCache pipelineCache,
			vk::RenderPass jobRenderPass,
			vk::PrimitiveTopology primitiveTopology,
			vk::PolygonMode polygonMode,
			vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo) {

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
				{}, primitiveTopology
		};

		std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{
				{}, dynamicStates.size(), dynamicStates.data()};


		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
				{}, 0, 0, polygonMode, vk::CullModeFlagBits::eNone,
				vk::FrontFace::eClockwise, 0,
				0, 0, 0, 1.0f
		};

		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
				{}, true, true,
				vk::CompareOp::eLessOrEqual,
				false, false,
				vk::StencilOpState{
						vk::StencilOp::eKeep, vk::StencilOp::eKeep,
						vk::StencilOp::eKeep, vk::CompareOp::eNever
				},
				vk::StencilOpState{
						vk::StencilOp::eKeep, vk::StencilOp::eKeep,
						vk::StencilOp::eKeep, vk::CompareOp::eAlways
				},
				0, 0
		};
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{
				true,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eDstAlpha,
				vk::BlendOp::eMax,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
				{}, false, vk::LogicOp::eCopy, 1,
				&pipelineColorBlendAttachmentState
		};
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
				{}, vk::SampleCountFlagBits::e1};

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
				{}, pipelineShaderStageCreateInfos.size(), pipelineShaderStageCreateInfos.data(),
				&pipelineVertexInputStateCreateInfo,
				&pipelineInputAssemblyStateCreateInfo,
				&pipelineTessellationStateCreateInfo,
				nullptr,
				&pipelineRasterizationStateCreateInfo,
				&pipelineMultisampleStateCreateInfo,
				&pipelineDepthStencilStateCreateInfo,
				&pipelineColorBlendStateCreateInfo,
				&pipelineDynamicStateCreateInfo,
				pipelineLayout,
				jobRenderPass
		};
		//return vk::UniquePipeline{};
		return get().createGraphicsPipelineUnique(pipelineCache, pipelineCreateInfo);
	}

	BufferMemory Device::flushBufferToDevMemory(vk::BufferUsageFlags bufferUsageFlags,
	                                            vk::MemoryPropertyFlags memoryPropertyFlags,
	                                            size_t size,
	                                            BufferMemory &&bufferMemory) {
		auto BAM = createBufferAndMemory(
				size,
				bufferUsageFlags,
				memoryPropertyFlags
		);

		auto copyCmd = createCmdBuffers(
				1, gPoolUnique.get(),
				[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
					commandBufferBeginHandle.copyBuffer(
							std::get<vk::UniqueBuffer>(bufferMemory).get(),
							std::get<vk::UniqueBuffer>(BAM).get(),
							{vk::BufferCopy{0, 0, size}});
				},
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		auto copyFence = submitCmdBuffer(copyCmd[0].get());
		waitFence(copyFence.get());

		return BAM;
	}

	void Device::flushBufferToBuffer(vk::Buffer srcbuffer, vk::Buffer decbuffer,
	                                 size_t size, size_t srcoff, size_t decoff) {
		auto copyCmd = createCmdBuffers(
				1, gPoolUnique.get(),
				[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
					commandBufferBeginHandle.copyBuffer(srcbuffer, decbuffer,
					                                    {vk::BufferCopy{srcoff, decoff, size}});
				},
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		);
		auto copyFence = submitCmdBuffer(copyCmd[0].get());
		waitFence(copyFence.get());
	}


	void Device::bindBsm(BuffersMemory<> &BsM) {
		for (auto &buffer:BsM.desAndBuffers()) {
			get().bindBufferMemory(buffer.buffer().get(), BsM.memory().get(), buffer.off());
		}
	}

	vk::UniqueDevice Device::initHander(vk::PhysicalDevice &phyDevice, vk::SurfaceKHR &surface) {
		std::array<float, 1> queuePriorities{0.0};
		std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
		auto queueFamilyProperties = phyDevice.getQueueFamilyProperties();
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {//fixme queue count priorities
			//MY_LOG(INFO) << "QueueFamilyProperties : " << i << "\tflags:"
			//              << vk::to_string(queueFamilyProperties[i].queueFlags);
			if (phyDevice.getSurfaceSupportKHR(i, surface) &&
			    (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
				MY_LOG(ERROR) << "fixme default_queue_index :" << i
				              << "\tgetSurfaceSupportKHR:true";
				deviceQueueCreateInfos.emplace_back(
						vk::DeviceQueueCreateFlagBits{}, i,
						queueFamilyProperties[i].queueCount, queuePriorities.data()
				);
				continue;
			}//todo multi queue
			//if(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer){}
			//if(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute){ }
		}

		gQueueFamilyIndex = deviceQueueCreateInfos[0].queueFamilyIndex;//FIXME QueueFamilyIndexs need manager

		auto deviceLayerProperties = phyDevice.enumerateDeviceLayerProperties();
		//auto deviceFeatures = phyDevice.getFeatures();
		//MY_LOG(INFO) << "deviceFeatures.samplerAnisotropy = "<<deviceFeatures.samplerAnisotropy;
		//MY_LOG(INFO) << "phyDeviceDeviceLayerProperties : " << deviceLayerProperties.size();
		std::vector<const char *> deviceLayerPropertiesName;
		for (auto &deviceLayerPropertie :deviceLayerProperties) {
			MY_LOG(INFO) << "phyDeviceDeviceLayerPropertie : " << deviceLayerPropertie.layerName;
			deviceLayerPropertiesName.emplace_back(deviceLayerPropertie.layerName);
		}
		auto deviceExtensionProperties = phyDevice.enumerateDeviceExtensionProperties();
		//for (auto &deviceExtensionPropertie:deviceExtensionProperties)
		//	MY_LOG(INFO) << "PhyDeviceExtensionPropertie : "
		//	             << deviceExtensionPropertie.extensionName;
		std::array deviceExtensionNames{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
				VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
				VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,
				//VK_EXT_DEBUG_MARKER_EXTENSION_NAME
				//VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
				//VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME
		};

		return phyDevice.createDeviceUnique(
				vk::DeviceCreateInfo{
						{},
						deviceQueueCreateInfos.size(),
						deviceQueueCreateInfos.data(),
						deviceLayerPropertiesName.size(),
						deviceLayerPropertiesName.data(),
						deviceExtensionNames.size(),
						deviceExtensionNames.data()
				}
		);
	}

	template<typename T>
	void buildCmdBufferHelper(tt::Window &window, const vk::RenderPass &renderPass, T &job) {
		job.buildCmdBuffer(window, renderPass);
	}

	template<typename T, typename ... Ts>
	void buildCmdBufferHelper(tt::Window &window, const vk::RenderPass &renderPass, T &job,
	                          Ts &... jobs) {
		buildCmdBufferHelper(window, renderPass, job);
		buildCmdBufferHelper(window, renderPass, jobs...);
	}

	void Device::buildCmdBuffer(tt::Window &window) {
		JobBase::getPerspective() = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(window.getSwapchainExtent().width) /
				static_cast<float>(window.getSwapchainExtent().height),
				0.1f, 256.0f
		);

		std::apply([&](auto &... jobs) { buildCmdBufferHelper(window, renderPass.get(), jobs...); },
		           Jobs);

		mainCmdBuffers = helper::createCmdBuffers(get(), renderPass.get(),
		                                          *this,
		                                          window.getFrameBuffer(),
		                                          window.getSwapchainExtent(),
		                                          commandPool.get());
	}


	template<typename T>
	void execSubCmdBufferHelper(RenderpassBeginHandle &handle, uint32_t frameIndex, T &job) {
		handle.executeCommands(job.getGraphisCmdBuffer(frameIndex));
	}

	template<typename T, typename ... Ts>
	void execSubCmdBufferHelper(RenderpassBeginHandle &handle, uint32_t frameIndex, T &job,
	                            Ts &... jobs) {
		execSubCmdBufferHelper(handle, frameIndex, job);
		execSubCmdBufferHelper(handle, frameIndex, jobs...);
	}

	void Device::CmdBufferBegin(CommandBufferBeginHandle &, vk::Extent2D, uint32_t frameIndex) {

	}

	void Device::CmdBufferRenderpassBegin(RenderpassBeginHandle &handle, vk::Extent2D,
	                                      uint32_t frameIndex) {
		std::apply(
				[&](auto &... jobs) { execSubCmdBufferHelper(handle, frameIndex, jobs...); },
				Jobs
		);
	}

	void Device::writeTextureToImage(ktx2 &texture, vk::Image image) {
		auto transferSrcBuffer = createStagingBufferMemoryOnObjs(texture.bufferSize());
		{
			auto sampleBufferPtr = mapBufferMemory(transferSrcBuffer);
			memcpy(sampleBufferPtr.get(), texture.bufferPtr(), texture.bufferSize());
			MY_LOG(ERROR)<<__func__<<"texture.bufferSize():"<<texture.bufferSize();
		}

		auto bufferCopyRegion = texture.copyRegions();
		/*
		for(auto& bufferCR:bufferCopyRegion){
			MY_LOG(ERROR)<<"bufferCopyRegion:"<<bufferCR.bufferOffset;
			MY_LOG(ERROR)<<"bufferCopyRegion:"<<bufferCR.imageExtent.width;
			MY_LOG(ERROR)<<"bufferCopyRegion:"<<bufferCR.imageExtent.height;
			MY_LOG(ERROR)<<"bufferCopyRegion:"<<bufferCR.imageExtent.depth;
			MY_LOG(ERROR)<<"bufferCopyRegion:"<<bufferCR.imageSubresource.layerCount;
		}*/
		auto subResourceRange = texture.imageSubresourceRange();
		//MY_LOG(ERROR)<<"subResourceRange levelCount:"<<subResourceRange.levelCount;
		//MY_LOG(ERROR)<<"subResourceRange layerCount:"<<subResourceRange.layerCount;

		vk::ImageMemoryBarrier imageMemoryBarrierToDest{
				{}, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 0, 0,
				image, subResourceRange
		};

		vk::ImageMemoryBarrier imageMemoryBarrierToGeneral{
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 0, 0,
				image, subResourceRange
		};
		auto copyCmd = createCmdBuffers(
				1, gPoolUnique.get(),
				[&](CommandBufferBeginHandle &commandBufferBeginHandle) {
					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eHost,
							vk::PipelineStageFlagBits::eTransfer,
							vk::DependencyFlags{},
							0, nullptr,
							0, nullptr,
							1, &imageMemoryBarrierToDest);

					commandBufferBeginHandle.copyBufferToImage(
							std::get<vk::UniqueBuffer>(transferSrcBuffer).get(),
							image,
							vk::ImageLayout::eTransferDstOptimal,
							bufferCopyRegion);

					commandBufferBeginHandle.pipelineBarrier(
							vk::PipelineStageFlagBits::eTransfer,
							vk::PipelineStageFlagBits::eFragmentShader,
							vk::DependencyFlags{},
							0, nullptr,
							0, nullptr,
							1, &imageMemoryBarrierToGeneral);
				},
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		);

		auto copyFence = submitCmdBuffer(copyCmd[0].get());
		waitFence(copyFence.get());

	}
}