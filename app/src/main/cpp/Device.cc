//
// Created by ttand on 19-8-2.
//

#include "Job.hh"
#include "Device.hh"
#include "Window.hh"

namespace tt{
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

	vk::UniquePipeline Device::createPipeline(uint32_t dataStepSize,
	                                          std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos,
	                                          vk::PipelineCache &pipelineCache,
	                                          vk::PipelineLayout pipelineLayout) {

		std::array vertexInputBindingDescriptions{
				vk::VertexInputBindingDescription{
						0, dataStepSize,
						vk::VertexInputRate::eVertex
				}
		};
		std::array vertexInputAttributeDescriptions{
				vk::VertexInputAttributeDescription{
						0, 0, vk::Format::eR32G32B32A32Sfloat, 0
				},
				vk::VertexInputAttributeDescription{
						1, 0, vk::Format::eR32G32Sfloat, 16
				}//VK_FORMAT_R32G32_SFLOAT
		};
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
				vk::PipelineVertexInputStateCreateFlags(),
				vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
				vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList
		};
		//vk::PipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo{};

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
				vk::PipelineViewportStateCreateFlags(),
				1, nullptr, 1, nullptr
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
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
		pipelineColorBlendAttachmentState.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
				vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eClear, 1,
				&pipelineColorBlendAttachmentState
		};
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
				vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1};

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
				vk::PipelineCreateFlags(),
				shaderStageCreateInfos.size(), shaderStageCreateInfos.data(),
				&pipelineVertexInputStateCreateInfo,
				&pipelineInputAssemblyStateCreateInfo,
				nullptr,
				&pipelineViewportStateCreateInfo,
				&pipelineRasterizationStateCreateInfo,
				&pipelineMultisampleStateCreateInfo,
				&pipelineDepthStencilStateCreateInfo,
				&pipelineColorBlendStateCreateInfo,
				&pipelineDynamicStateCreateInfo,
				pipelineLayout,
				renderPass.get()
		};
		return get().createGraphicsPipelineUnique(pipelineCache, pipelineCreateInfo);
	}

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
	Device::createCmdBuffers(tt::Window &swapchain, vk::CommandPool pool,
	                         std::function<void(RenderpassBeginHandle &)> functionRenderpassBegin,
	                         std::function<void(CommandBufferBeginHandle &)> functionBegin) {
		MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << swapchain.getFrameBufferNum();
		std::vector commandBuffers = get().allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo{
						pool,
						vk::CommandBufferLevel::ePrimary,
						swapchain.getFrameBufferNum()
				}
		);

		std::array clearValues{
				vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.2f, 0.2f}}},
				vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}},
		};
		uint32_t frameIndex = 0;
		for (auto &cmdBuffer : commandBuffers) {
			//cmdBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
			{
				CommandBufferBeginHandle cmdBeginHandle{cmdBuffer};
				functionBegin(cmdBeginHandle);
				{
					RenderpassBeginHandle cmdHandleRenderpassBegin{
							cmdBeginHandle,
							vk::RenderPassBeginInfo{
									renderPass.get(),
									swapchain.getFrameBuffer()[frameIndex].get(),
									vk::Rect2D{
											vk::Offset2D{},
											swapchain.getSwapchainExtent()
									},
									clearValues.size(), clearValues.data()
							}
					};
					functionRenderpassBegin(cmdHandleRenderpassBegin);
				}

			}
			++frameIndex;
		}
		return commandBuffers;
	}

	std::vector<vk::UniqueCommandBuffer>
	Device::createCmdBuffers(size_t cmdNum, vk::CommandPool pool,
	                         std::function<void(CommandBufferBeginHandle &)> functionBegin,
	                         vk::CommandBufferUsageFlags commandBufferUsageFlags) {
		MY_LOG(INFO) << ":allocateCommandBuffersUnique:" << cmdNum;
		std::vector<vk::UniqueCommandBuffer> commandBuffers = get().allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo{pool, vk::CommandBufferLevel::ePrimary, cmdNum});
		for (auto &cmdBuffer : commandBuffers) {
			CommandBufferBeginHandle cmdBeginHandle{cmdBuffer,commandBufferUsageFlags};
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
	Device::createImageAndMemoryFromMemory(gli::texture2d t2d, vk::ImageUsageFlags imageUsageFlags) {
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
		MY_LOG(INFO)<<"t2d Format:"<<vk::to_string(static_cast<vk::Format >(t2d.format()));
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
				imageSubresourceRange);
		{
			auto transferSrcBuffer = createBufferAndMemory(
					t2d.size(),
					vk::BufferUsageFlagBits::eTransferSrc,
					vk::MemoryPropertyFlagBits::eHostVisible |
					vk::MemoryPropertyFlagBits::eHostCoherent);
			{
				auto sampleBufferPtr = mapMemoryAndSize(transferSrcBuffer);
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

	BufferMemory Device::createBufferAndMemoryFromAssets(android_app *androidAppCtx,std::vector<std::string> names,
	                                                     vk::BufferUsageFlags bufferUsageFlags,
	                                                     vk::MemoryPropertyFlags memoryPropertyFlags) {
		auto alignment = phyDevice().getProperties().limits.minStorageBufferOffsetAlignment;
		off_t bufferLength = 0;
		std::vector <std::tuple<AAssetHander,off_t ,off_t >> fileHanders;
		for(auto&name:names){
			auto file = AAssetManagerFileOpen(androidAppCtx->activity->assetManager,name);
			if(!file)
				throw std::runtime_error{"asset not found"};
			auto length =  AAsset_getLength(file.get());
			fileHanders.emplace_back(std::move(file),bufferLength,length);
			length += (alignment - 1) - ((length - 1) % alignment) ;
			bufferLength += length;
		}
		if(memoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
			auto BAM = createBufferAndMemory(bufferLength,vk::BufferUsageFlagBits::eTransferSrc,vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);
			auto bufferPtr = mapMemoryAndSize(BAM);
			for(auto&file:fileHanders){
				AAsset_read(std::get<0>(file).get(),(char*)bufferPtr.get()+std::get<1>(file),std::get<2>(file));
			}
			auto BAM2 = createBufferAndMemory(bufferLength,bufferUsageFlags,memoryPropertyFlags);
			auto copyCmd = createCmdBuffers(
					1, gPoolUnique.get(),
					[&](CommandBufferBeginHandle &commandBufferBeginHandle){
						commandBufferBeginHandle.copyBuffer(std::get<vk::UniqueBuffer>(BAM).get(),
								std::get<vk::UniqueBuffer>(BAM2).get(),
								{vk::BufferCopy{0,0,bufferLength}});
					},
					vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

			auto copyFence = submitCmdBuffer(copyCmd[0].get());
			waitFence(copyFence.get());
			return BAM2;
		}

		auto BAM = createBufferAndMemory(bufferLength,bufferUsageFlags,memoryPropertyFlags);
		auto bufferPtr = mapMemoryAndSize(BAM);
		for(auto&file:fileHanders){
			AAsset_read(std::get<0>(file).get(),(char*)bufferPtr.get()+std::get<1>(file),std::get<2>(file));
		}
		return BAM;
	}

	Job Device::createJob(std::vector<vk::DescriptorPoolSize> descriptorPoolSizes,
	                       std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings) {

		return tt::Job{*this, gQueueFamilyIndex, std::move(descriptorPoolSizes),
		               std::move(descriptorSetLayoutBindings)};
	}
}