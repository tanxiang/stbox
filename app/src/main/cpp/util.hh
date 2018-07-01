//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_UTIL_H
#define STBOX_UTIL_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <unistd.h>
#include <android/log.h>
#include "vulkan.hpp"
#include "main.hh"
#include <thread>
#include <queue>
#include <condition_variable>
#include <android_native_app_glue.h>
#include <iostream>

std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader);

#define SWAPCHAIN_NUM 2
namespace tt {
    class Device : public vk::Device {
        vk::PhysicalDevice &physicalDevice;
        uint32_t queueFamilyIndex;
        vk::UniqueCommandPool commandPool;
        std::vector<vk::UniqueCommandBuffer> commandBuffers;
        vk::UniqueSwapchainKHR swapchainKHR;
        vk::Extent2D swapchainExtent;
        vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
        std::vector<std::tuple<vk::Image, vk::UniqueImageView, vk::UniqueFramebuffer, vk::UniqueFence>> vkSwapChainBuffers;
        vk::UniqueImage depthImage;
        vk::UniqueImageView depthImageView;
        vk::UniqueDeviceMemory depthImageMemory;
        std::array<vk::UniqueBuffer, SWAPCHAIN_NUM> mvpBuffer{
                createBufferUnique(
                        vk::BufferCreateInfo{
                                vk::BufferCreateFlags(),
                                sizeof(glm::mat4),
                                vk::BufferUsageFlagBits::eUniformBuffer}),
                createBufferUnique(
                        vk::BufferCreateInfo{
                                vk::BufferCreateFlags(),
                                sizeof(glm::mat4),
                                vk::BufferUsageFlagBits::eUniformBuffer})};
        std::array<vk::MemoryRequirements, SWAPCHAIN_NUM> mvpBufferMemoryRqs{
                getBufferMemoryRequirements(mvpBuffer[0].get()),
                getBufferMemoryRequirements(mvpBuffer[1].get())

        };
        std::array<vk::UniqueDeviceMemory, SWAPCHAIN_NUM> mvpMemorys{
                allocateMemoryUnique(vk::MemoryAllocateInfo{
                                             mvpBufferMemoryRqs[0].size,
                                             findMemoryTypeIndex(mvpBufferMemoryRqs[0].memoryTypeBits,
                                                                 vk::MemoryPropertyFlags() |
                                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                                 vk::MemoryPropertyFlagBits::eHostCoherent)
                                     }
                ),
                allocateMemoryUnique(vk::MemoryAllocateInfo{
                                             mvpBufferMemoryRqs[1].size,
                                             findMemoryTypeIndex(mvpBufferMemoryRqs[1].memoryTypeBits,
                                                                 vk::MemoryPropertyFlags() |
                                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                                 vk::MemoryPropertyFlagBits::eHostCoherent)
                                     }
                )
        };


        vk::UniqueDescriptorSetLayout ttcreateDescriptorSetLayoutUnique() {
            std::array<vk::DescriptorSetLayoutBinding, 2> descriptSlBs{
                    vk::DescriptorSetLayoutBinding{
                            0,
                            vk::DescriptorType::eUniformBuffer,
                            1,
                            vk::ShaderStageFlagBits::eVertex
                    },
                    vk::DescriptorSetLayoutBinding{
                            1,
                            vk::DescriptorType::eCombinedImageSampler,
                            1,
                            vk::ShaderStageFlagBits::eFragment
                    }
            };
            return createDescriptorSetLayoutUnique(
                    vk::DescriptorSetLayoutCreateInfo{
                            vk::DescriptorSetLayoutCreateFlags(), descriptSlBs.size(),
                            descriptSlBs.data()
                    });
        }

        vk::UniqueDescriptorSetLayout descriptorSetLayout{ttcreateDescriptorSetLayoutUnique()};

        vk::UniquePipelineLayout pipelineLayout = createPipelineLayoutUnique(
                vk::PipelineLayoutCreateInfo{
                        vk::PipelineLayoutCreateFlags(), 1, &descriptorSetLayout.get(), 0, nullptr
                });

        vk::UniqueDescriptorPool ttcreateDescriptorPoolUnique() {
            std::array<vk::DescriptorPoolSize, 2> poolSize{
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 2},
                    vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 2}};
            return createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                    vk::DescriptorPoolCreateFlags(), 2, poolSize.size(), poolSize.data()});
        }

        vk::UniqueDescriptorPool descriptorPoll = ttcreateDescriptorPoolUnique();

        std::vector<vk::UniqueDescriptorSet> ttcreateDescriptorSets() {
            std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts{descriptorSetLayout.get(),
                                                                        descriptorSetLayout.get()};
            return allocateDescriptorSetsUnique(
                    vk::DescriptorSetAllocateInfo{
                            descriptorPoll.get(), descriptorSetLayouts.size(),
                            descriptorSetLayouts.data()});
        }

        std::vector<vk::UniqueDescriptorSet> descriptorSets{ttcreateDescriptorSets()};
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineCache vkPipelineCache = createPipelineCacheUnique(
                vk::PipelineCacheCreateInfo{});
        vk::UniquePipeline graphicsPipeline;

        std::unique_ptr<std::thread> submitThread;
        std::queue<uint32_t> frameSubmitIndex;
        bool submitExitFlag = false;
        std::mutex mutexDraw;
        std::condition_variable cvDraw;

        uint32_t findMemoryTypeIndex(uint32_t memoryTypeBits, vk::MemoryPropertyFlags flags);

        vk::UniqueShaderModule loadShaderFromFile(const char *filePath,
                                                  android_app *androidAppCtx);

    public:
        Device(vk::Device dev, vk::PhysicalDevice &phy, uint32_t qidx) :
                vk::Device{dev}, physicalDevice{phy}, queueFamilyIndex{qidx},
                commandPool{createCommandPoolUnique(
                        vk::CommandPoolCreateInfo{
                                vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qidx})
                }, commandBuffers{allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{commandPool.get(),
                                                                           vk::CommandBufferLevel::ePrimary, 2})} {
            auto mvpBufferInfo0 = vk::DescriptorBufferInfo{mvpBuffer[0].get(), 0,
                                                           sizeof(glm::mat4)};
            bindBufferMemory(mvpBuffer[0].get(), mvpMemorys[0].get(), 0);
            auto mvpBufferInfo1 = vk::DescriptorBufferInfo{mvpBuffer[1].get(), 0,
                                                           sizeof(glm::mat4)};
            bindBufferMemory(mvpBuffer[1].get(), mvpMemorys[1].get(), 0);

            assert(descriptorSets.size() == 2);
            std::cout << "descriptorSets.size()" << descriptorSets.size() << std::endl;
            updateDescriptorSets(
                    std::vector<vk::WriteDescriptorSet>{
                            vk::WriteDescriptorSet{descriptorSets[0].get(), 0, 0, 1,
                                                   vk::DescriptorType::eUniformBuffer,
                                                   nullptr, &mvpBufferInfo0},
                            vk::WriteDescriptorSet{descriptorSets[1].get(), 0, 0, 1,
                                                   vk::DescriptorType::eUniformBuffer,
                                                   nullptr, &mvpBufferInfo1}},

                    nullptr);    //todo use_texture
        }

        Device(Device &&odevice) : physicalDevice{odevice.physicalDevice},
                                   queueFamilyIndex{odevice.queueFamilyIndex},
                                   commandPool{std::move(odevice.commandPool)},
                                   commandBuffers{std::move(odevice.commandBuffers)},
                                   swapchainKHR{std::move(odevice.swapchainKHR)},
                                   swapchainExtent{std::move(odevice.swapchainExtent)},
                                   depthFormat{odevice.depthFormat},
                                   vkSwapChainBuffers{std::move(odevice.vkSwapChainBuffers)},
                                   depthImage{std::move(odevice.depthImage)},
                                   depthImageView{std::move(odevice.depthImageView)},
                                   depthImageMemory{std::move(odevice.depthImageMemory)},
                                   mvpBuffer{std::move(odevice.mvpBuffer)},
                                   mvpMemorys{std::move(odevice.mvpMemorys)},
                                   descriptorSetLayout{std::move(odevice.descriptorSetLayout)},
                                   pipelineLayout{std::move(odevice.pipelineLayout)},
                                   descriptorPoll{std::move(odevice.descriptorPoll)},
                                   descriptorSets{std::move(odevice.descriptorSets)},
                                   renderPass{std::move(odevice.renderPass)},
                                   vkPipelineCache{std::move(odevice.vkPipelineCache)},
                                   graphicsPipeline{std::move(odevice.graphicsPipeline)} {}

        Device() = delete;

        ~Device() {
            waitIdle();
            destroy();
        }

        vk::Extent2D getSwapchainExtent() {
            return swapchainExtent;
        }

        std::vector<vk::UniqueCommandBuffer> &defaultPoolAllocBuffer() {
            return commandBuffers;
        }

        vk::SurfaceFormatKHR getSurfaceDefaultFormat(vk::SurfaceKHR &surfaceKHR);

        vk::UniqueDeviceMemory allocBindImageMemory(vk::Image image, vk::MemoryPropertyFlags flags);

        vk::UniqueDeviceMemory allocMemoryAndWrite(vk::Buffer &buffer, void *pData, size_t dataSize,
                                                   vk::MemoryPropertyFlags memoryPropertyFlags);

        void buildSwapchainViewBuffers(vk::SurfaceKHR &surfaceKHR);

        //void buildMVPBufferAndWrite(glm::mat4 MVP);

        void updateMVPBuffer(glm::mat4 MVP);

        void buildRenderpass(vk::SurfaceKHR &surfaceKHR);

        void buildPipeline(uint32_t dataStepSize, android_app *app);

        //void renderPassReset() {
        //    vkSwapChainBuffers.clear();
        //    renderPass.reset();
        //}

        uint32_t
        drawCmdBuffer(glm::mat4 MVP, vk::Buffer vertexBuffer);

        void buildSubmitThread(vk::SurfaceKHR &surfaceKHR);

        void stopSubmitThread();

        void resetDraw() {
            while (!frameSubmitIndex.empty()) {
                swapchainPresent();
            }
        }

        void swapchainPresent();
        void swapchainPresentSync();

    };

    class Instance : public vk::Instance {
        std::vector<vk::PhysicalDevice> vkPhysicalDevices = enumeratePhysicalDevices();
        std::unique_ptr<tt::Device> upDevice;
        vk::UniqueSurfaceKHR surfaceKHR;
        bool focus = false;
    public:
        Instance(vk::Instance &&ins) : vk::Instance{std::move(ins)} {

        }

        Instance(Instance &&i) : vk::Instance{std::move(i)},
                                 vkPhysicalDevices{std::move(i.vkPhysicalDevices)},
                                 surfaceKHR{std::move(i.surfaceKHR)},
                                 upDevice{std::move(i.upDevice)} {

        }

        vk::PhysicalDevice &defaultPhyDevice() {
            return vkPhysicalDevices[0];
        }

        tt::Device &defaultDevice() {
            return *upDevice;
        }

        vk::SurfaceKHR &defaultSurface() {
            return surfaceKHR.get();
        }

        uint32_t queueFamilyPropertiesFindFlags(vk::QueueFlags);

        void connectWSI(ANativeWindow *window) {
            surfaceKHR = createAndroidSurfaceKHRUnique(
                    vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
                                                    window});
        }

        void disconnectWSI() {
            upDevice->resetDraw();
            surfaceKHR.reset();
        }

        tt::Device connectToDevice();

        bool connectedDevice() {
            return upDevice.operator bool();
        }

        void connectDevice();

        void disconnectDevice() {
            upDevice.reset();
        }

        void setFocus() {
            focus = true;
        }

        void unsetFocus() {
            focus = false;
        }

        bool isFocus() {
            return focus;
        }

        ~Instance() {
            destroy();
        }

    };

    Instance createInstance();

}

class Camera
{
private:
    float fov;
    float znear, zfar;

    void updateViewMatrix()
    {
        glm::mat4 rotM = glm::mat4(1.0f);
        glm::mat4 transM;

        rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        transM = glm::translate(glm::mat4(1.0f), position);

        if (type == CameraType::firstperson)
        {
            matrices.view = rotM * transM;
        }
        else
        {
            matrices.view = transM * rotM;
        }

        updated = true;
    };
public:
    enum CameraType { lookat, firstperson };
    CameraType type = CameraType::lookat;

    glm::vec3 rotation = glm::vec3();
    glm::vec3 position = glm::vec3();

    float rotationSpeed = 1.0f;
    float movementSpeed = 1.0f;

    bool updated = false;

    struct
    {
        glm::mat4 perspective;
        glm::mat4 view;
    } matrices;

    struct
    {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } keys;

    bool moving()
    {
        return keys.left || keys.right || keys.up || keys.down;
    }

    float getNearClip() {
        return znear;
    }

    float getFarClip() {
        return zfar;
    }

    void setPerspective(float fov, float aspect, float znear, float zfar)
    {
        this->fov = fov;
        this->znear = znear;
        this->zfar = zfar;
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    };

    void updateAspectRatio(float aspect)
    {
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    }

    void setPosition(glm::vec3 position)
    {
        this->position = position;
        updateViewMatrix();
    }

    void setRotation(glm::vec3 rotation)
    {
        this->rotation = rotation;
        updateViewMatrix();
    };

    void rotate(glm::vec3 delta)
    {
        this->rotation += delta;
        updateViewMatrix();
    }

    void setTranslation(glm::vec3 translation)
    {
        this->position = translation;
        updateViewMatrix();
    };

    void translate(glm::vec3 delta)
    {
        this->position += delta;
        updateViewMatrix();
    }

    void update(float deltaTime)
    {
        updated = false;
        if (type == CameraType::firstperson)
        {
            if (moving())
            {
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
                    position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
                if (keys.right)
                    position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

                updateViewMatrix();
            }
        }
    };

    // Update camera passing separate axis data (gamepad)
    // Returns true if view or position has been changed
    bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
    {
        bool retVal = false;

        if (type == CameraType::firstperson)
        {
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
            if (fabsf(axisLeft.y) > deadZone)
            {
                float pos = (fabsf(axisLeft.y) - deadZone) / range;
                position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
                retVal = true;
            }
            if (fabsf(axisLeft.x) > deadZone)
            {
                float pos = (fabsf(axisLeft.x) - deadZone) / range;
                position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
                retVal = true;
            }

            // Rotate
            if (fabsf(axisRight.x) > deadZone)
            {
                float pos = (fabsf(axisRight.x) - deadZone) / range;
                rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
                retVal = true;
            }
            if (fabsf(axisRight.y) > deadZone)
            {
                float pos = (fabsf(axisRight.y) - deadZone) / range;
                rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
                retVal = true;
            }
        }
        else
        {
            // todo: move code from example base class for look-at
        }

        if (retVal)
        {
            updateViewMatrix();
        }

        return retVal;
    }

};
#endif //STBOX_UTIL_H
