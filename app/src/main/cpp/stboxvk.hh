//
// Created by ttand on 18-2-12.
//

#ifndef STBOX_STBOXVK_HH
#define STBOX_STBOXVK_HH

#include "util.hh"
#include "onnx.hh"
#include "main.hh"

//uint32_t draw_run(tt::Device &ttInstance,vk::SurfaceKHR &surfaceKHR);
namespace tt {
	class stboxvk {
        std::unique_ptr<tt::Device> devicePtr;
        std::unique_ptr<tt::Window> windowPtr;

    public:
        void initData(android_app *app, tt::Instance &instance);

        void initDevice(android_app *app,tt::Instance &instance,vk::PhysicalDevice &physicalDevice,vk::SurfaceKHR surface);

        void initWindow(android_app *app, tt::Instance &instance);

        Job& initJobs(android_app *app,tt::Device &device);

        void cleanWindow();

        explicit operator bool() const {
            return devicePtr && windowPtr;
        }
    };
}
#endif //STBOX_STBOXVK_HH
