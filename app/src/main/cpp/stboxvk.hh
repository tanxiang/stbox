//
// Created by ttand on 18-2-12.
//

#ifndef STBOX_STBOXVK_HH
#define STBOX_STBOXVK_HH

#include "util.hh"

//uint32_t draw_run(tt::Device &ttInstance,vk::SurfaceKHR &surfaceKHR);
namespace tt {
    class stboxvk {
        std::unique_ptr<tt::Device> devicePtr;
        std::unique_ptr<tt::Swapchain> swapchainPtr;


    public:
        void initWindow(android_app *app, tt::Instance &instance);

        void cleanWindow();

        explicit operator bool() const {
            return devicePtr && swapchainPtr;
        }
    };
}
#endif //STBOX_STBOXVK_HH
