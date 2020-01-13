//
// Created by ttand on 18-2-12.
//

#ifndef STBOX_STBOXVK_HH
#define STBOX_STBOXVK_HH

#include "util.hh"
#include "onnx.hh"
#include "Window.hh"
#include "JobDrawLine.hh"

//uint32_t draw_run(tt::Device &ttInstance,vk::SurfaceKHR &surfaceKHR);
namespace tt {
	class JobDraw;
	class JobFont;
	class stboxvk {
        std::unique_ptr<Device> devices;
		//std::vector<JobDraw> drawJobs;
		//std::vector<JobDrawLine> drawLineJobs;
		//std::vector<JobFont> fontJobs;
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
