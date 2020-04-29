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
	};
}

#endif //STBOX_KTX2_HH
