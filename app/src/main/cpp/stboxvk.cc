//
// Created by ttand on 18-2-12.
//
#include "vulkan.hpp"

#include "stboxvk.hh"
#include <iostream>
//#include <cassert>
#include "main.hh"

#include "util.hh"
#include "Instance.hh"
#include "Device.hh"
#include "JobFont.hh"
//#include "onnx.hh"
#include <functional>


vk::Extent2D AndroidGetWindowSize(android_app *Android_application) {
	// On Android, retrieve the window size from the native window.
	assert(Android_application != nullptr);
	return vk::Extent2D{ANativeWindow_getWidth(Android_application->window),
	                    ANativeWindow_getHeight(Android_application->window)};
}

namespace tt {

	void stboxvk::initData(android_app *app, tt::Instance &instance) {
//		Onnx nf{"/storage/0123-4567/nw/mobilenetv2-1.0.onnx"};
	}

	void stboxvk::initDevice(android_app *app, tt::Instance &instance,
	                         vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR surface) {
		devices = std::make_unique<Device>(physicalDevice, surface, app);//reconnect
	}

	void stboxvk::initWindow(android_app *app, tt::Instance &instance) {
		assert(instance);
		//initData(app,instance);
		auto surface = instance.connectToWSI(app->window);

		if (!devices) {
			auto phyDevices = instance->enumeratePhysicalDevices()[0];
			auto phyFeatures = phyDevices.getFeatures();
			MY_LOG(INFO) << "geometryShader : " << phyFeatures.geometryShader;

			auto Properties2 = phyDevices.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceSubgroupProperties>(
					instance.extProcDispatch());
			auto subgroupProperties = Properties2.get<vk::PhysicalDeviceSubgroupProperties>();
			MY_LOG(INFO) << vk::to_string(subgroupProperties.supportedOperations)
			             << subgroupProperties.quadOperationsInAllStages;
			initDevice(app, instance, phyDevices, surface.get());
		}

		auto &window = windows.emplace_back(std::move(surface), *devices,
		                                    AndroidGetWindowSize(app));
		devices->buildCmdBuffer(window);
		devices->flushMVP();
	}

	void stboxvk::draw() {
		devices->runJobOnWindow(windows[0]);
	}

	void stboxvk::draw(float dx, float dy) {
		JobBase::setRotate(dx, dy);
		devices->flushMVP();
		draw();
	}

	void stboxvk::cleanWindow() {
		windows.clear();
	}


}
