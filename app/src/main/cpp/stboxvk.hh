//
// Created by ttand on 18-2-12.
//

#ifndef STBOX_STBOXVK_HH
#define STBOX_STBOXVK_HH

#include "util.hh"
#include "onnx.hh"
#include "main.hh"
#include "JobDraw.hh"
#include "JobFont.hh"

//uint32_t draw_run(tt::Device &ttInstance,vk::SurfaceKHR &surfaceKHR);
namespace tt {
	class stboxvk {
        std::vector<Device> devices;
		std::vector<JobDraw> drawJobs;
		std::vector<JobFont> fontJobs;
		std::vector<Window> windows;

    public:
        void initData(android_app *app, tt::Instance &instance);

		Device& initDevice(android_app *app,tt::Instance &instance,vk::PhysicalDevice &physicalDevice,vk::SurfaceKHR surface);

        void initWindow(android_app *app, tt::Instance &instance);

        //void initJobs(android_app *app,tt::Device &device);
		//void initJobs2(android_app *app,tt::Device &device);

        void cleanWindow();

		void draw();

		void draw(float dx,float dy);

		//void draw(glm::mat4 &cam);
        //explicit operator bool() const {
        //    return devices && windows;
        //}
    };
}
#endif //STBOX_STBOXVK_HH
