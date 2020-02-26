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
				vk::ShaderModuleCreateFlags(), fileContent.size(),
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
						vk::AttachmentDescriptionFlags(),
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
						vk::AttachmentDescriptionFlags(),
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
						vk::SubpassDescriptionFlags(),
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
				vk::RenderPassCreateFlags(),
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
						vk::ImageCreateFlags(),
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
				vk::ImageViewCreateInfo{vk::ImageViewCreateFlags(),
				                        std::get<vk::UniqueImage>(IVM).get(),
				                        vk::ImageViewType::e2D,
				                        format,
				                        componentMapping,
				                        imageSubresourceRange});
		return IVM;
	}


	ImageViewMemory
	Device::createImageAndMemoryFromMemory(gli::texture2d t2d,
	                                       vk::ImageUsageFlags imageUsageFlags) {
		vk::ImageSubresourceRange imageSubresourceRange{
				vk::ImageAspectFlagBits::eColor,
				0, t2d.levels(), 0, 1
		};
		//todo check t2d.format();
		auto imageAndMemory = createImageAndMemory(
				vk::Format::eR8G8B8A8Unorm, vk::Extent3D{
						static_cast<uint32_t>(t2d[0].extent().x),
						static_cast<uint32_t>(t2d[0].extent().y),
						1
				},
				imageUsageFlags | vk::ImageUsageFlagBits::eTransferDst,
				t2d.levels(),
				vk::ComponentMapping{
						vk::ComponentSwizzle::eR,
						vk::ComponentSwizzle::eG,
						vk::ComponentSwizzle::eB,
						vk::ComponentSwizzle::eA
				},
				imageSubresourceRange);

		return imageAndMemory;
	}

	ImageViewMemory
	Device::createImageAndMemoryFromT2d(gli::texture2d t2d, vk::ImageUsageFlags imageUsageFlags) {
		vk::ImageSubresourceRange imageSubresourceRange{
				vk::ImageAspectFlagBits::eColor,
				0, t2d.levels(), 0, 1
		};
		//todo check t2d.format();
		//MY_LOG(INFO) << "t2d Format:" << vk::to_string(static_cast<vk::Format >(t2d.format()));
		auto imageAndMemory = createImageAndMemory(
				static_cast<vk::Format >(t2d.format()), vk::Extent3D{
						static_cast<uint32_t>(t2d[0].extent().x),
						static_cast<uint32_t>(t2d[0].extent().y),
						1
				},
				imageUsageFlags | vk::ImageUsageFlagBits::eTransferDst,
				t2d.levels(),
				vk::ComponentMapping{
						vk::ComponentSwizzle::eR,
						vk::ComponentSwizzle::eG,
						vk::ComponentSwizzle::eB,
						vk::ComponentSwizzle::eA
				},
				imageSubresourceRange
		);
		{
			auto transferSrcBuffer = createStagingBufferMemoryOnObjs(t2d.size());
			{
				auto sampleBufferPtr = mapBufferMemory(transferSrcBuffer);
				memcpy(sampleBufferPtr.get(), t2d.data(), t2d.size());
			}

			vk::ImageMemoryBarrier imageMemoryBarrierToDest{
					vk::AccessFlags{}, vk::AccessFlagBits::eTransferWrite,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 0, 0,
					std::get<vk::UniqueImage>(imageAndMemory).get(),
					imageSubresourceRange
			};
			std::vector<vk::BufferImageCopy> bufferCopyRegion;
			for (int i = 0, offset = 0; i < t2d.levels(); ++i) {
				MY_LOG(INFO) << "BufferImageCopy" << t2d[i].extent().x << 'X' << t2d[i].extent().y;
				bufferCopyRegion.emplace_back(
						offset, 0, 0,
						vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, i, 0, 1},
						vk::Offset3D{},
						vk::Extent3D{t2d[i].extent().x, t2d[i].extent().y, 1});
				offset += t2d[i].size();
			}

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
								std::get<vk::UniqueImage>(imageAndMemory).get(),
								vk::ImageLayout::eTransferDstOptimal,
								bufferCopyRegion);
						vk::ImageMemoryBarrier imageMemoryBarrierToGeneral{
								vk::AccessFlagBits::eTransferWrite,
								vk::AccessFlagBits::eShaderRead,
								vk::ImageLayout::eTransferDstOptimal,
								vk::ImageLayout::eShaderReadOnlyOptimal, 0, 0,
								std::get<vk::UniqueImage>(imageAndMemory).get(),
								imageSubresourceRange
						};
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
		return imageAndMemory;
	}

	BufferMemory
	Device::createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
	                              vk::MemoryPropertyFlags memoryPropertyFlags) {

		BufferMemory BM{};
		std::get<vk::UniqueBuffer>(BM) = get().createBufferUnique(
				vk::BufferCreateInfo{
						vk::BufferCreateFlags(),
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
	Device::createLocalBufferMemory(size_t dataSize,vk::BufferUsageFlags flags){
		vk::UniqueBuffer buffer = get().createBufferUnique(
				vk::BufferCreateInfo{
						vk::BufferCreateFlags(),
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

		get().bindBufferMemory(buffer.get(),memory.get(),0);
		return LocalBufferMemory{std::move(buffer),std::move(memory)};
	}


	BufferMemory Device::createBufferAndMemoryFromAssets(android_app *androidAppCtx,
	                                                     std::vector<std::string> names,
	                                                     vk::BufferUsageFlags bufferUsageFlags,
	                                                     vk::MemoryPropertyFlags memoryPropertyFlags) {
		auto alignment = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
		off_t offset = 0;
		std::vector<std::tuple<AAssetHander, off_t, off_t >> fileHanders;
		for (auto &name:names) {
			auto file = AAssetManagerFileOpen(androidAppCtx->activity->assetManager, name);
			if (!file)
				throw std::runtime_error{"asset not found"};
			auto length = AAsset_getLength(file.get());
			fileHanders.emplace_back(std::move(file), offset, length);
			length += (alignment - 1) - ((length - 1) % alignment);
			offset += length;
		}
		if (memoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
			auto StagingBufferMemory = createStagingBufferMemoryOnObjs(offset);
			{
				auto bufferPtr = mapBufferMemory(StagingBufferMemory);
				for (auto &file:fileHanders) {
					AAsset_read(std::get<0>(file).get(),
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
						commandBufferBeginHandle.copyBuffer(std::get<vk::UniqueBuffer>(StagingBufferMemory).get(),
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
			AAsset_read(std::get<0>(file).get(), (char *) bufferPtr.get() + std::get<1>(file),
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
			vk::PrimitiveTopology primitiveTopology) {

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
				vk::PipelineInputAssemblyStateCreateFlags(), primitiveTopology
		};

		std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{
				vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data()};


		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
				vk::PipelineRasterizationStateCreateFlags(),
				0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
				vk::FrontFace::eClockwise, 0,
				0, 0, 0, 1.0f
		};

		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
				vk::PipelineDepthStencilStateCreateFlags(),
				true, true,
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
				vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eCopy, 1,
				&pipelineColorBlendAttachmentState
		};
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
				vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1};

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
				vk::PipelineCreateFlags(),
				pipelineShaderStageCreateInfos.size(), pipelineShaderStageCreateInfos.data(),
				&pipelineVertexInputStateCreateInfo,
				&pipelineInputAssemblyStateCreateInfo,
				nullptr,
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

	void Device::flushBufferToMemory(vk::Buffer buffer, vk::DeviceMemory memory, size_t size,
	                                 size_t srcoff, size_t decoff) {
		auto bufferDst = get().createBufferUnique(
				vk::BufferCreateInfo{
						vk::BufferCreateFlags(),
						size,
						vk::BufferUsageFlagBits::eTransferDst}
		);
		get().bindBufferMemory(bufferDst.get(), memory, 0);
		return flushBufferToBuffer(buffer, bufferDst.get(), size, srcoff, decoff);
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
						vk::DeviceQueueCreateFlags(), i,
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
						vk::DeviceCreateFlags(),
						deviceQueueCreateInfos.size(),
						deviceQueueCreateInfos.data(),
						deviceLayerPropertiesName.size(),
						deviceLayerPropertiesName.data(),
						deviceExtensionNames.size(),
						deviceExtensionNames.data()
				}
		);
	}
}