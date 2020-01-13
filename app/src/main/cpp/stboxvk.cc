//
// Created by ttand on 18-2-12.
//
#include "vulkan.hpp"

#include "stboxvk.hh"
#include <iostream>
//#include <cassert>
#include "main.hh"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gli/gli.hpp>

#include "util.hh"
#include "Instance.hh"
#include "Device.hh"
#include "JobFont.hh"
#include "JobDraw.hh"
#include "onnx.hh"
#include <functional>


vk::Extent2D AndroidGetWindowSize(android_app *Android_application) {
	// On Android, retrieve the window size from the native window.
	assert(Android_application != nullptr);
	return vk::Extent2D{ANativeWindow_getWidth(Android_application->window),
	                    ANativeWindow_getHeight(Android_application->window)};
}

namespace tt {

	void stboxvk::initData(android_app *app, tt::Instance &instance) {
		Onnx nf{"/storage/0123-4567/nw/mobilenetv2-1.0.onnx"};
	}

	Device &stboxvk::initDevice(android_app *app, tt::Instance &instance,
	                            vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR surface) {
		return devices.emplace_back(
				instance.connectToDevice(physicalDevice, surface,app));//reconnect
	}

	void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
		assert(instance);
		//initData(app,instance);
		auto surface = instance.connectToWSI(app->window);

		if (devices.empty()) {
			auto phyDevices = instance->enumeratePhysicalDevices()[0];
			auto phyFeatures = phyDevices.getFeatures();
			MY_LOG(INFO) << "geometryShader : " << phyFeatures.geometryShader;
			//phyDevices[0].getSurfaceCapabilities2KHR(vk::PhysicalDeviceSurfaceInfo2KHR{surface.get()});
			//auto graphicsQueueIndex = queueFamilyPropertiesFindFlags(phyDevices[0],
			//                                                         vk::QueueFlagBits::eGraphics,
			//                                                         surface.get());
			auto &device = initDevice(app, instance, phyDevices, surface.get());
			//

			//drawJobs.emplace_back(JobDraw::create(app, device));
			//fontJobs.emplace_back(JobFont::create(app, device));
			/*drawLineJobs.emplace_back(
					device.createJobBase(
							{
									vk::DescriptorPoolSize{
											vk::DescriptorType::eUniformBuffer, 1
									},
									vk::DescriptorPoolSize{
											vk::DescriptorType::eStorageBuffer, 3
									}
							},
							2
					),
					app,
					device);*/
		}

		auto &window = windows.emplace_back(std::move(surface), devices[0],
		                                    AndroidGetWindowSize(app));
		devices[0].Job<JobDraw>().buildCmdBuffer(window, devices[0].renderPass.get());
		devices[0].Job<JobDraw>().setPv();
		//fontJobs[0].buildCmdBuffer(window, devices[0].renderPass.get());
		return;
	}

	void stboxvk::draw() {
		devices[0].runJobOnWindow(devices[0].Job<JobDraw>(), windows[0]);
		//devices[0].runJobOnWindow(fontJobs[0], windows[0]);
	}

	void stboxvk::draw(float dx, float dy) {
		devices[0].Job<JobDraw>().setPv(dx, dy);
		draw();
	}

	void stboxvk::cleanWindow() {
		//MY_LOG(INFO) << __func__ ;
		windows.clear();
		//devices.reset();
	}


}
