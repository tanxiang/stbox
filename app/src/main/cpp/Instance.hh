//
// Created by ttand on 19-8-2.
//

#ifndef STBOX_INSTANCE_HH
#define STBOX_INSTANCE_HH

#include "util.hh"

namespace tt {
	class Device;
	class Instance : public vk::UniqueInstance {
	public:
		Instance() {

		}

		Instance(vk::UniqueInstance &&ins) : vk::UniqueInstance{std::move(ins)} {

		}


		auto connectToWSI(ANativeWindow *window) {
			return get().createAndroidSurfaceKHRUnique(
					vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
					                                window});
		}


		tt::Device
		connectToDevice(vk::PhysicalDevice &phyDevice, vk::SurfaceKHR &surface);

	};
	Instance createInstance();
}

#endif //STBOX_INSTANCE_HH
