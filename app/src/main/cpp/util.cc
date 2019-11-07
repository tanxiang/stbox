//
// Created by ttand on 18-2-11.
//
#include <assert.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "util.hh"

// Header files.
#include <android_native_app_glue.h>
#include <cstring>
//#include "shaderc/shaderc.hpp"
#include "util.hh"
#include "stboxvk.hh"
#include <map>

#include "vertexdata.hh"
#include <algorithm>
#include <cstring>
#include <array>

#include "stb_font_consolas_24_latin1.inl"

using namespace std::chrono_literals;

/*
static const std::map<vk::ShaderStageFlagBits, shaderc_shader_kind> shader_map_table{
        {vk::ShaderStageFlagBits::eVertex,                 shaderc_glsl_vertex_shader},
        {vk::ShaderStageFlagBits::eTessellationControl,    shaderc_glsl_tess_control_shader},
        {vk::ShaderStageFlagBits::eTessellationEvaluation, shaderc_glsl_tess_evaluation_shader},
        {vk::ShaderStageFlagBits::eGeometry,               shaderc_glsl_geometry_shader},
        {vk::ShaderStageFlagBits::eFragment,               shaderc_glsl_fragment_shader},
        {vk::ShaderStageFlagBits::eCompute,                shaderc_glsl_compute_shader},
};

//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
std::vector<uint32_t> GLSLtoSPV(const vk::ShaderStageFlagBits shader_type, const char *pshader) {
    // On Android, use shaderc instead.
    shaderc::Compiler compiler;
    shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(pshader, strlen(pshader), shader_map_table.at(shader_type),
                                      "shader");
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error_msg{"shaderc Error: Id=%d, Msg=%s"};
        error_msg += module.GetCompilationStatus();
        error_msg += module.GetErrorMessage().c_str();
        throw std::runtime_error{error_msg};
    }
    return std::vector<uint32_t>{module.cbegin(), module.cend()};
}
*/

#include <android/log.h>


namespace tt {
	namespace helper {
	}

	uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice, uint32_t memoryTypeBits,
	                             vk::MemoryPropertyFlags flags) {
		auto memoryProperties = physicalDevice.getMemoryProperties();
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			//MY_LOG(INFO) << vk::to_string(memoryProperties.memoryTypes[i].propertyFlags)
			//             << " march " << vk::to_string(flags) << " ? " << i;
			if ((memoryTypeBits & 1) == 1) {
				// Type is available, does it match user properties?

				if ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
					return i;
				}

			}
			memoryTypeBits >>= 1;
		}
		MY_LOG(INFO) << "memoryProperties no match exit!";
		exit(-1);//todo throw
	}

	std::vector<char> loadDataFromAssets(const std::string &filePath,
	                                     android_app *androidAppCtx) {
		// Read the file
		assert(androidAppCtx);
		auto file = AAssetManagerFileOpen(androidAppCtx->activity->assetManager,filePath);
		std::vector<char> fileContent;
		fileContent.resize(AAsset_getLength(file.get()));
		AAsset_read(file.get(), reinterpret_cast<void *>(fileContent.data()), fileContent.size());
		return fileContent;
	}


	StbFontChar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];

}