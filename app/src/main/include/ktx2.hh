//
// Created by ttand on 20-4-28.
//

#ifndef STBOX_KTX2_HH
#define STBOX_KTX2_HH
#include <ss.hh>
#include "vulkan.hpp"
#include <vector>

namespace tt {
	class ktx2 {
		void *textureKtx;
	public:
		ktx2(const ktx2&)= delete;
		ktx2& operator=( const ktx2& ) = delete;
		ktx2(const unsigned char* bytes, int size);
		~ktx2();
		vk::ImageCreateInfo vkImageCI();
		vk::ImageViewCreateInfo vkImageViewCI(vk::Image);
		std::vector<vk::BufferImageCopy> copyRegions();
		vk::ImageSubresourceRange imageSubresourceRange();
		size_t bufferSize();
		unsigned char* bufferPtr();
		vk::Format format();
		size_t numLevels();
		size_t numLayers();
		size_t numLayersAll();
		bool isCubemap();
#ifdef KTX_VK_LOAD
		void debugLoad(vk::PhysicalDevice physicalDevice,vk::Device device,vk::Queue queue,vk::CommandPool commandPool);
		vk::Image debugIMG();
		vk::Format debugFMT();
		vk::ImageViewType debugVT();
		size_t debugLevelC();
		size_t debugLayerC();
#endif
	};
}

#endif //STBOX_KTX2_HH
