//
// Created by ttand on 19-8-2.
//

#include "Instance.hh"
#include "JobBase.hh"
#include "Device.hh"


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
		VkDebugReportFlagsEXT msgFlags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject, size_t location,
		int32_t msgCode, const char *pLayerPrefix,
		const char *pMsg, void *pUserData) {
	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		__android_log_print(ANDROID_LOG_ERROR,
		                    "Stbox",
		                    "ERROR: [%s] Code %i : %s",
		                    pLayerPrefix, msgCode, pMsg);
	} else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		__android_log_print(ANDROID_LOG_WARN,
		                    "Stbox",
		                    "WARNING: [%s] Code %i : %s",
		                    pLayerPrefix, msgCode, pMsg);
	} else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		__android_log_print(ANDROID_LOG_WARN,
		                    "Stbox",
		                    "PERFORMANCE WARNING: [%s] Code %i : %s",
		                    pLayerPrefix, msgCode, pMsg);
	} else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		__android_log_print(ANDROID_LOG_INFO,
		                    "Stbox", "INFO: [%s] Code %i : %s",
		                    pLayerPrefix, msgCode, pMsg);
	} else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		__android_log_print(ANDROID_LOG_VERBOSE,
		                    "Stbox", "DEBUG: [%s] Code %i : %s",
		                    pLayerPrefix, msgCode, pMsg);
	}

	// Returning false tells the layer not to stop when the event occurs, so
	// they see the same behavior with and without validation layers enabled.
	return VK_FALSE;
}

VkDebugReportCallbackEXT debugReportCallback;

namespace tt{
	tt::Instance createInstance() {
		auto instanceLayerProperties = vk::enumerateInstanceLayerProperties();
		MY_LOG(INFO) << "enumerateInstanceLayerProperties:" << instanceLayerProperties.size();
		std::vector<const char *> instanceLayerPropertiesName;

		//for (auto &prop:instanceLayerProperties) {
			//MY_LOG(INFO) << prop.layerName;
			//instanceLayerPropertiesName.emplace_back(prop.layerName);
		//}
		vk::ApplicationInfo vkAppInfo{"stbox", VK_VERSION_1_0, "stbox",
		                              VK_VERSION_1_0, VK_API_VERSION_1_0};

		std::vector instanceEtensionNames{
				VK_KHR_SURFACE_EXTENSION_NAME,
				VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
				//VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
				//VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		};
		auto instanceExts = vk::enumerateInstanceExtensionProperties();
		for (auto &Ext: instanceExts) {
			//MY_LOG(INFO) << "instanceExt:" << Ext.extensionName;
			if (!std::strcmp(Ext.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
				//MY_LOG(INFO) << "push " << Ext.extensionName;
				//instanceEtensionNames.emplace_back(Ext.extensionName);
			}
		}

		vk::InstanceCreateInfo instanceInfo{
				vk::InstanceCreateFlags(), &vkAppInfo,
				instanceLayerPropertiesName.size(),
				instanceLayerPropertiesName.data(),
				instanceEtensionNames.size(),
				instanceEtensionNames.data()
		};
		auto ins = vk::createInstanceUnique(instanceInfo);
		for (auto &ExtName:instanceEtensionNames)
			if (!std::strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, ExtName)) {
				auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) (ins->getProcAddr(
						"vkCreateDebugReportCallbackEXT"));
				vk::DebugReportCallbackCreateInfoEXT debugReportInfo{
						vk::DebugReportFlagBitsEXT::eError |
						vk::DebugReportFlagBitsEXT::eWarning |
						vk::DebugReportFlagBitsEXT::ePerformanceWarning |
						vk::DebugReportFlagBitsEXT::eDebug |
						vk::DebugReportFlagBitsEXT::eInformation,
						DebugReportCallback};
				vkCreateDebugReportCallbackEXT((VkInstance) ins.get(),
				                               (VkDebugReportCallbackCreateInfoEXT *) &debugReportInfo,
				                               nullptr, &debugReportCallback);
				MY_LOG(ERROR) << "vkCreateDebugReportCallbackEXT";
			}


		//vkCreateDebugReportCallbackEXT((VkInstance)ins.get(),(VkDebugReportCallbackCreateInfoEXT*)&debugReportInfo, nullptr,&debugReportCallback);
		//auto vkDestroyDebugReportCallbackEXT = static_cast<PFN_vkDestroyDebugReportCallbackEXT>(ins->getProcAddr("vkDestroyDebugReportCallbackEXT"));
		//assert(vkCreateDebugReportCallbackEXT);
		//assert(vkDestroyDebugReportCallbackEXT);
		return tt::Instance{std::move(ins)};
	}
}