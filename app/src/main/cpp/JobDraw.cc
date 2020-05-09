//
// Created by ttand on 19-9-25.
//

#include "JobDraw.hh"
#include "vertexdata.hh"
#include "Device.hh"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

namespace tt {

	JobDraw JobDraw::create(android_app *app, tt::Device &device) {
		return JobDraw{
				device.createJobBase(
						{
								vk::DescriptorPoolSize{
										vk::DescriptorType::eUniformBuffer, 1
								},
								vk::DescriptorPoolSize{
										vk::DescriptorType::eCombinedImageSampler, 1
								},
						}, 1
				),
				app,
				device
		};
	}

	vk::UniquePipeline
	JobDraw::createPipeline(Device &device, android_app *app, vk::PipelineLayout pipelineLayout) {

		auto vertShaderModule = device.loadShaderFromAssets("shaders/mvp.vert.spv", app);
		auto fargShaderModule = device.loadShaderFromAssets("shaders/copy.frag.spv", app);
		std::array pipelineShaderStageCreateInfos
				{
						vk::PipelineShaderStageCreateInfo{
								{},
								vk::ShaderStageFlagBits::eVertex,
								vertShaderModule.get(),
								"main"
						},
						vk::PipelineShaderStageCreateInfo{
								{},
								vk::ShaderStageFlagBits::eFragment,
								fargShaderModule.get(),
								"main"
						}
				};


		std::array vertexInputBindingDescriptions{
				vk::VertexInputBindingDescription{
						0, sizeof(VertexUV),
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
				{}, vertexInputBindingDescriptions.size(), vertexInputBindingDescriptions.data(),
				vertexInputAttributeDescriptions.size(), vertexInputAttributeDescriptions.data()

		};
		return device.createGraphsPipeline(pipelineShaderStageCreateInfos,
		                                   pipelineVertexInputStateCreateInfo,
		                                   pipelineLayout,
		                                   pipelineCache.get(), device.renderPass.get(),
		                                   vk::PrimitiveTopology::eTriangleFan);

	}

	void JobDraw::buildCmdBuffer(tt::Window &swapchain, vk::RenderPass renderPass) {
		cmdBuffers = helper::createCmdBuffersSub(descriptorPool.getOwner(), renderPass,
		                                         *this,
		                                         swapchain.getFrameBuffer(),
		                                         swapchain.getSwapchainExtent(),
		                                         commandPool.get());
		//setPerspective(swapchain);
	}

	/*
	void JobDraw::setPerspective(tt::Window &swapchain) {
		perspective = glm::perspective(
				glm::radians(60.0f),
				static_cast<float>(swapchain.getSwapchainExtent().width) /
				static_cast<float>(swapchain.getSwapchainExtent().height),
				0.1f, 256.0f
		);
	}
	*/
	namespace glmx {
		using namespace glm;

		template<typename T, qualifier Q>
		GLM_FUNC_QUALIFIER mat<4, 4, T, Q>
		lookcc(vec<2, T, Q> const& xy) {
			vec<3, T, Q> const up{0,1,0};
			vec<3, T, Q> const eye{xy.x,xy.y,1};
			vec<3, T, Q> const f(normalize(vec<3, T, Q>{} - eye));
			vec<3, T, Q> const s(normalize(cross(f, up)));
			vec<3, T, Q> const u(cross(s, f));
			mat<4, 4, T, Q> Result(0);
			Result[0][0] = s.x;
			Result[1][0] = s.y;
			Result[2][0] = s.z;
			Result[0][1] = u.x;
			Result[1][1] = u.y;
			Result[2][1] = u.z;
			Result[0][2] = -f.x;
			Result[1][2] = -f.y;
			Result[2][2] = -f.z;
			Result[3][3] =1;
			return Result;
		}

	}

	void JobDraw::setPv(float dx, float dy) {
		auto nlookat = lookat*glmx::lookcc(glm::vec2(dx*0.01,dy*0.01));
		static float datx=0.0,daty=0.0;
		datx+=dx*0.01;
		daty+=dy*0.01;
		glm::vec3 eulerAngle{-daty, datx, 0.0};


		auto c = glm::cos(eulerAngle * 0.5f);
		auto s = glm::sin(eulerAngle * 0.5f);

		glm::qua<float> fRotate{
			c.x * c.y * c.z - s.x * s.y * s.z,
			s.x * c.y * c.z - c.x * s.y * s.z,
			c.x * s.y * c.z + s.x * c.y * s.z,
			c.x * c.y * s.z + s.x * s.y * c.z
		};
				//lookat = nlookat;
		helper::mapTypeMemoryAndSize<glm::mat4>(ownerDevice(), BAMs[0])[0] =
				perspective * lookat * glm::mat4_cast(fRotate) ;
	}

	JobDraw::JobDraw(JobBase &&j, android_app *app, tt::Device &device) :
			JobBase{std::move(j)},
			graphPipeline{
					device.get(),
					descriptorPool.get(),
					[&](vk::PipelineLayout pipelineLayout) {
						return createPipeline(device, app, pipelineLayout);
					},{},
					std::array{
							vk::DescriptorSetLayoutBinding{
									0, vk::DescriptorType::eUniformBuffer,
									1, vk::ShaderStageFlagBits::eVertex
							},
							vk::DescriptorSetLayoutBinding{
									1, vk::DescriptorType::eCombinedImageSampler,
									1, vk::ShaderStageFlagBits::eFragment
							}
					}
			} {
		BAMs.emplace_back(
				device.createBufferAndMemory(
						sizeof(glm::mat4),
						vk::BufferUsageFlagBits::eUniformBuffer |
						vk::BufferUsageFlagBits::eTransferSrc,
						vk::MemoryPropertyFlagBits::eHostVisible |
						vk::MemoryPropertyFlagBits::eHostCoherent));

		std::vector<VertexUV> vertices{
				{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}},
				{{-.5f, .5f,  0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-.5f, -.5f, 0.0f, 1.0f}, {0.0f, 0.0f}},
				{{.5f,  -.5f, 0.0f, 1.0f}, {1.0f, 0.0f}}
		};
		std::array indexes{0u, 1u, 2u, 3u};
		device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eVertexBuffer |
		                             vk::BufferUsageFlagBits::eIndexBuffer, vertices, indexes);
		//device.buildBufferOnBsM(Bsm, vk::BufferUsageFlagBits::eIndexBuffer, indexes);
		{
			auto localeBufferMemory = device.createStagingBufferMemoryOnObjs(vertices, indexes);
			{
				uint32_t off = 0;
				auto memoryPtr = device.mapBufferMemory(localeBufferMemory);
				off += device.writeObjsDescriptorBufferInfo(
						memoryPtr, Bsm.desAndBuffers()[0], off,
						vertices, indexes);
			}
			device.buildMemoryOnBsM(Bsm, vk::MemoryPropertyFlagBits::eDeviceLocal);
			device.flushBufferToMemory(std::get<vk::UniqueBuffer>(localeBufferMemory).get(),
			                           Bsm.memory().get(), Bsm.size());
		}

		{
			auto fileContent = loadDataFromAssets("textures/ic_launcher-web.ktx", app);
			//gli::texture2d tex2d;
			auto tex2d = gli::texture2d{gli::load_ktx((char*)fileContent.data(), fileContent.size())};
			sampler = device.createSampler(tex2d.levels());
			IVMs.emplace_back(device.createImageAndMemoryFromT2d(tex2d));
		}


		//uniquePipeline = createPipeline(device, app);

		auto descriptorBufferInfo = //createDescriptorBufferInfoTuple(device.Job<JobDrawLine>().memoryWithParts, 3);
				device.getDescriptorBufferInfo(BAMs[0]);
		auto descriptorImageInfo = device.getDescriptorImageInfo(IVMs[0], sampler.get());

		std::array writeDes{
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 0, 0, 1,
						vk::DescriptorType::eUniformBuffer,
						nullptr, &descriptorBufferInfo
				},
				vk::WriteDescriptorSet{
						graphPipeline.getDescriptorSet(), 1, 0, 1,
						vk::DescriptorType::eCombinedImageSampler,
						&descriptorImageInfo
				}
		};
		device->updateDescriptorSets(writeDes, nullptr);
		//MY_LOG(INFO)<<__FUNCTION__<<" run out";
	}

	void
	JobDraw::CmdBufferRenderPassContinueBegin(CommandBufferBeginHandle &cmdHandleRenderpassContinue,
	                                          vk::Extent2D win, uint32_t frameIndex) {
		cmdHandleRenderpassContinue.setViewport(
				0,
				std::array{
						vk::Viewport{
								0, 0,
								win.width,
								win.height,
								0.0f, 1.0f
						}
				}
		);

		cmdHandleRenderpassContinue.setScissor(0, std::array{vk::Rect2D{{}, win}});
		cmdHandleRenderpassContinue.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.get());
		cmdHandleRenderpassContinue.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				graphPipeline.layout(), 0,
				graphPipeline.getDescriptorSets(),
				{});
		std::array offsets{vk::DeviceSize{0}};
		cmdHandleRenderpassContinue.bindVertexBuffers(
				0,
				Bsm.desAndBuffers()[0].buffer().get(),
				offsets);
		cmdHandleRenderpassContinue.bindIndexBuffer(
				Bsm.desAndBuffers()[0].buffer().get(),
				Bsm.desAndBuffers()[0].descriptors()[1].offset, vk::IndexType::eUint32);
		cmdHandleRenderpassContinue.drawIndexed(4, 1, 0, 0, 0);
	}
};