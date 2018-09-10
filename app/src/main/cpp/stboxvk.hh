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
public:
    void init(android_app *app,tt::Instance &instance){
        assert(instance);
        auto surface = instance.connectToWSI(app->window);
        device = instance.connectToDevice(surface.get());
        swapchain = tt::Swapchain{std::move(surface),device};
        //swapchain.reset(new tt::Swapchain{std::move(surface),*device});

        device.buildCmdBuffers(vk::Buffer{},swapchain);
    }
    void term(){
        swapchain.reset();
        device.reset();
    }
    explicit operator bool () const{
        return device && swapchain;
    }
};
#endif //STBOX_STBOXVK_HH
