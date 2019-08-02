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

/*
        Instance(Instance &&i) : vk::UniqueInstance{std::move(i)} {

        }

        Instance & operator=(Instance && instance)
        {
            vk::UniqueInstance::operator=(std::move(instance));
            return *this;
        }
*/
		auto connectToWSI(ANativeWindow *window) {
			return get().createAndroidSurfaceKHRUnique(
					vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
					                                window});
		}

		std::unique_ptr <tt::Device>
		connectToDevice(vk::PhysicalDevice &phyDevice, vk::SurfaceKHR &surface);

	};
}

#endif //STBOX_INSTANCE_HH
