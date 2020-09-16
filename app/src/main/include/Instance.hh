//
// Created by ttand on 19-8-2.
//

#pragma once

#include "util.hh"

namespace tt {
	class Device;

	class DispatchLoaderExt {
	public:
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
		PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;

		DispatchLoaderExt(vk::Instance instance) :
				vkCreateDebugReportCallbackEXT{
						reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
								instance.getProcAddr(
										"vkCreateDebugReportCallbackEXT")
						)
				},
				vkGetPhysicalDeviceProperties2{
						reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
								instance.getProcAddr(
										"vkGetPhysicalDeviceProperties2")
						)
				} {

		}
	};

	class Instance : public vk::UniqueInstance {
		DispatchLoaderExt dispatchLoaderExt;
	public:

		Instance(vk::UniqueInstance &&ins) : vk::UniqueInstance{std::move(ins)},
		                                     dispatchLoaderExt{get()} {

		}

		auto connectToWSI(ANativeWindow *window) {
			return get().createAndroidSurfaceKHRUnique(
					vk::AndroidSurfaceCreateInfoKHR{vk::AndroidSurfaceCreateFlagsKHR(),
					                                window});
		}

		auto &extProcDispatch() {
			return dispatchLoaderExt;
		}

	};

	Instance createInstance();

}

