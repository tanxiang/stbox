//
// Created by ttand on 18-2-12.
//

#ifndef STBOX_STBOXVK_HH
#define STBOX_STBOXVK_HH

#include "util.hh"

//uint32_t draw_run(tt::Device &ttInstance,vk::SurfaceKHR &surfaceKHR);
class stboxvk{
    tt::Device device;
    tt::Swapchain swapchain;
    tt::Device::BufferViewMemory mvpBuffer,vertexBuffer,indexBuffer;
    std::vector<vk::UniqueCommandBuffer> mianBuffers;
public:
    void init(android_app *app,tt::Instance &instance);
    void clean();
    explicit operator bool () const{
        return device && swapchain;
    }
};
#endif //STBOX_STBOXVK_HH
