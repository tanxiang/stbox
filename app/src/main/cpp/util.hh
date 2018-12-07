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
#include <android/log.h>
#include <vulkan.hpp>
#include "main.hh"
#include <thread>
#include <queue>
#include <condition_variable>
#include <android_native_app_glue.h>
#include <iostream>
#include <memory>

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

#define SWAPCHAIN_NUM 2

namespace tt {
    class Swapchain;
    std::vector<char> loadDataFromAssets(const char *filePath,android_app *androidAppCtx);
    uint32_t queueFamilyPropertiesFindFlags(vk::PhysicalDevice PhyDevice, vk::QueueFlags flags,
                                            vk::SurfaceKHR surface);

    struct RenderpassBeginHandle: public vk::CommandBuffer{
        RenderpassBeginHandle(vk::CommandBuffer commandBuffer,vk::RenderPassBeginInfo renderPassBeginInfo)
                :vk::CommandBuffer{commandBuffer}{
            beginRenderPass(renderPassBeginInfo,vk::SubpassContents::eInline);
        }
        ~RenderpassBeginHandle(){
            endRenderPass();
        }
        RenderpassBeginHandle( const RenderpassBeginHandle& ) = delete; // non construction-copyable
        RenderpassBeginHandle& operator=( const RenderpassBeginHandle& ) = delete; // non copyable
    };

    struct CommandBufferBeginHandle: public vk::CommandBuffer {
        CommandBufferBeginHandle(vk::UniqueCommandBuffer &uniqueCommandBuffer):vk::CommandBuffer{uniqueCommandBuffer.get()}{
            begin(vk::CommandBufferBeginInfo{});
        }
        ~CommandBufferBeginHandle(){
            end();
        }

        CommandBufferBeginHandle( const CommandBufferBeginHandle& ) = delete; // non construction-copyable
        CommandBufferBeginHandle& operator=( const CommandBufferBeginHandle& ) = delete; // non copyable
    };

    class Device : public vk::UniqueDevice {
    public:
        using ImageViewMemory = std::tuple<vk::UniqueImage, vk::UniqueImageView, vk::UniqueDeviceMemory>;

        using BufferViewMemory = std::tuple<vk::UniqueBuffer, vk::UniqueDeviceMemory, size_t>;

        using BufferViewMemoryPtr = std::unique_ptr<void, std::function<void(void *)> >;


        BufferViewMemory mvpBuffer, vertexBuffer, indexBuffer;

    private:
        vk::PhysicalDevice physicalDevice;
        uint32_t queueFamilyIndex;
        vk::UniqueDescriptorPool descriptorPoll = ttcreateDescriptorPoolUnique();
        vk::UniquePipelineCache pipelineCache = get().createPipelineCacheUnique(vk::PipelineCacheCreateInfo{});
        vk::UniqueCommandPool commandPool = get().createCommandPoolUnique(vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex});

    public:

        vk::UniquePipeline graphicsPipeline;
        std::vector<vk::UniqueCommandBuffer> mianBuffers;
        std::vector<vk::UniqueDescriptorSet> descriptorSets;//{createDescriptorSets()};

    private:
        //std::vector<vk::UniqueCommandBuffer> commandBuffers;

        //std::vector<vk::UniqueFence> commandBufferFences;
        //struct {
            //Swap chain image presentation
            //vk::Semaphore presentComplete;
            //Command buffer submission and execution
            //vk::Semaphore renderComplete;
        //} semaphores;




        vk::UniqueDescriptorPool ttcreateDescriptorPoolUnique() {
            std::array<vk::DescriptorPoolSize,3> poolSize{
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
                    vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2},
                    vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 2}
            };
            return get().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                    vk::DescriptorPoolCreateFlags(), 3, poolSize.size(), poolSize.data()});
        }

        uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);

        vk::UniqueShaderModule loadShaderFromAssets(const char *filePath,
                                                    android_app *androidAppCtx);

    public:
        //Device(){}

        Device(vk::UniqueDevice &&dev, vk::PhysicalDevice &phy, uint32_t qidx) :
                vk::UniqueDevice{std::move(dev)}, physicalDevice{phy}, queueFamilyIndex{qidx}
                {}

        auto phyDevice() {
            return physicalDevice;
        }

        auto getCommandPool(){
            return commandPool.get();
        }

        auto transQueue(){
            return get().getQueue(queueFamilyIndex, 0);
        }

        auto compQueue(){
            return get().getQueue(queueFamilyIndex, 0);
        }

        auto graphsQueue(){
            return get().getQueue(queueFamilyIndex, 0);
        }

        vk::UniqueDescriptorSetLayout createDescriptorSetLayoutUnique(std::vector<vk::DescriptorSetLayoutBinding> descriptSlBs) {
            return get().createDescriptorSetLayoutUnique(
                    vk::DescriptorSetLayoutCreateInfo{
                            vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(),
                            descriptSlBs.data()
                    });
        }

        vk::UniquePipelineLayout  createPipelineLayout(vk::UniqueDescriptorSetLayout &descriptorSetLayout) {
            return get().createPipelineLayoutUnique(
                    vk::PipelineLayoutCreateInfo{
                            vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0,
                            nullptr
                    });
        }

        auto createDescriptorSets(vk::UniqueDescriptorSetLayout &descriptorSetLayout) {
            std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts{descriptorSetLayout.get()};
            return get().allocateDescriptorSetsUnique(
                    vk::DescriptorSetAllocateInfo{
                            descriptorPoll.get(), descriptorSetLayouts.size(),
                            descriptorSetLayouts.data()});
        }

        bool checkSurfaceSupport(vk::SurfaceKHR &surface){
            auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(physicalDevice,
                                                                     vk::QueueFlagBits::eGraphics,
                                                                     surface);
            if(graphicsQueueIndex == queueFamilyIndex)
                return true;
            return false;
        }

        vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

        ImageViewMemory createImageAndMemory(vk::Format format, vk::Extent3D extent3D,
                                             vk::ImageUsageFlags imageUsageFlags =
                                             vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                             vk::ImageUsageFlagBits::eTransientAttachment,
                                             uint32_t mipLevels = 1,
                                             vk::ComponentMapping componentMapping = vk::ComponentMapping{},
                                             vk::ImageSubresourceRange imageSubresourceRange = vk::ImageSubresourceRange{
                                                     vk::ImageAspectFlagBits::eDepth |
                                                     vk::ImageAspectFlagBits::eStencil,
                                                     0, 1, 0, 1});

        BufferViewMemory
        createBufferAndMemory(size_t dataSize, vk::BufferUsageFlags bufferUsageFlags,
                              vk::MemoryPropertyFlags memoryPropertyFlags);

        auto createSampler(uint32_t levels){
            return get().createSamplerUnique(vk::SamplerCreateInfo {vk::SamplerCreateFlags(),
                                                                    vk::Filter::eLinear,vk::Filter::eLinear,
                                                                    vk::SamplerMipmapMode::eLinear,
                                                                    vk::SamplerAddressMode::eRepeat,
                                                                    vk::SamplerAddressMode::eRepeat,
                                                                    vk::SamplerAddressMode::eRepeat,
                                                                    0,
                                                                    phyDevice().getFeatures().samplerAnisotropy,
                                                                    phyDevice().getProperties().limits.maxSamplerAnisotropy,
                                                                    0,vk::CompareOp::eNever,0,levels,vk::BorderColor::eFloatOpaqueWhite,0});
        }

        template<typename Tuple>
        auto mapMemoryAndSize(Tuple &tupleMemoryAndSize, size_t offset = 0){
            return BufferViewMemoryPtr{
                    get().mapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get(),
                                    offset,
                                    std::get<size_t>(tupleMemoryAndSize),
                                    vk::MemoryMapFlagBits()),
                    [this, &tupleMemoryAndSize](void *pVoid) {
                        get().unmapMemory(std::get<vk::UniqueDeviceMemory>(tupleMemoryAndSize).get());
                    }
            };
        }

        template<typename Tuple>
        auto getDescriptorBufferInfo(Tuple &tupleMemoryAndSize,vk::DeviceSize size = VK_WHOLE_SIZE,vk::DeviceSize offset = 0){
            return vk::DescriptorBufferInfo{std::get<vk::UniqueBuffer>(tupleMemoryAndSize).get(),offset,size};
        }

        template<typename Tuple>
        auto getDescriptorImageInfo(Tuple &tupleImage,vk::Sampler &sampler){
            return vk::DescriptorImageInfo{
                    sampler,std::get<vk::UniqueImageView >(tupleImage).get(),vk::ImageLayout::eShaderReadOnlyOptimal
            };
        }

        vk::UniquePipeline createPipeline(uint32_t dataStepSize, android_app *app, Swapchain &swapchain,
                            vk::PipelineLayout pipelineLayout);


        std::vector<vk::UniqueCommandBuffer>
        createCmdBuffers(size_t cmdNum,
                         std::function<void(CommandBufferBeginHandle&)> = [](CommandBufferBeginHandle&){});

        std::vector<vk::UniqueCommandBuffer>
        createCmdBuffers(tt::Swapchain &swapchain,
                         std::function<void(RenderpassBeginHandle&)> =[](RenderpassBeginHandle&){},
                         std::function<void(CommandBufferBeginHandle&)> = [](CommandBufferBeginHandle&){});


        vk::UniqueFence submitCmdBuffer(vk::CommandBuffer &commandBuffer){
            auto fence = get().createFenceUnique(vk::FenceCreateInfo{});
            std::array<vk::SubmitInfo, 1> submitInfos{
                    vk::SubmitInfo{
                            0, nullptr, nullptr,
                            1, &commandBuffer
                    }
            };
            return fence;
        }

        vk::UniqueFence submitCmdBuffer(Swapchain &swapchain,
                                        std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers,
                                        vk::Semaphore &imageAcquiredSemaphore,vk::Semaphore &renderSemaphore);

        void submitCmdBufferAndWait(Swapchain &swapchain,
                                    std::vector<vk::UniqueCommandBuffer> &drawcommandBuffers);

        auto waitFence(vk::Fence& Fence){
            return get().waitForFences(1,&Fence,true,10000000);
        }

    };

    class Swapchain : public vk::UniqueSwapchainKHR {
        vk::UniqueSurfaceKHR surface;
        vk::Extent2D swapchainExtent;
        vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
        std::vector<vk::UniqueImageView> imageViews;
        Device::ImageViewMemory depth;
        std::vector<vk::UniqueFramebuffer> frameBuffers;
        vk::UniqueRenderPass renderPass;

        void createRenderpass(Device &device);

    public:
        //Swapchain(){}

        Swapchain(vk::UniqueSurfaceKHR &&sf, tt::Device &device);

        vk::Extent2D getSwapchainExtent() {
            return swapchainExtent;
        }

        auto getRenderPass() {
            return renderPass.get();
        }

        auto getSwapchainImageNum() {
            return imageViews.size();
        }

        auto getFrameBufferNum() {
            return frameBuffers.size();
        }

        auto& getFrameBuffer() {
            return frameBuffers;
        }

        auto acquireNextImage(Device &device,vk::Semaphore presentCompleteSemaphore){
            return device->acquireNextImageKHR(get(),UINT64_MAX,presentCompleteSemaphore,vk::Fence{});
        }

        auto queuePresent(vk::Queue& queue,uint32_t imageIndex,vk::Semaphore waitSemaphore = vk::Semaphore{}){
            vk::PresentInfoKHR presentInfo{
                    0, nullptr,1,&get(),&imageIndex,

            };
            if(waitSemaphore) {
                presentInfo.pWaitSemaphores = &waitSemaphore;
                presentInfo.waitSemaphoreCount = 1;
            }
            return queue.presentKHR(presentInfo);
        }

    };

    class Instance : public vk::UniqueInstance {
    public:
        Instance(){

        }

        Instance(vk::UniqueInstance &&ins) : vk::UniqueInstance{std::move(ins)} {

        }
/*
        Instance(Instance &&i) : vk::UniqueInstance{std::move(i)} {

        }

        Instance & operator=(Instance && instance)
        {
            vk::UniqueInstance::operator=(std::move(instance));
            return *this;
        }
*/
        auto connectToWSI(ANativeWindow *window) {
            return get().createAndroidSurfaceKHRUnique(
                    vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
                                                    window});
        }

        std::unique_ptr<tt::Device> connectToDevice(vk::SurfaceKHR surface);
    };

    tt::Instance createInstance();

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
