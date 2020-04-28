//
// Created by ttand on 20-4-28.
//

#include "ktx2.hh"

#include <ktxvulkan.h>


typedef struct user_cbdata_optimal {
	VkBufferImageCopy* region; // Specify destination region in final image.
	VkDeviceSize offset;       // Offset of current level in staging buffer
	ktx_uint32_t numFaces;
	ktx_uint32_t numLayers;
	// The following are used only by optimalTilingPadCallback
	ktx_uint8_t* dest;         // Pointer to mapped staging buffer.
	ktx_uint32_t elementSize;
	ktx_uint32_t numDimensions;
#if defined(_DEBUG)
	VkBufferImageCopy* regionsArrayEnd;
#endif
} user_cbdata_optimal;

static KTX_error_code
optimalTilingCallback(int miplevel, int face,
                      int width, int height, int depth,
                      ktx_uint64_t faceLodSize,
                      void* pixels, void* userdata)
{
	user_cbdata_optimal* ud = (user_cbdata_optimal*)userdata;

	// Set up copy to destination region in final image
	//assert(ud->region < ud->regionsArrayEnd);
	ud->region->bufferOffset = ud->offset;
	ud->offset += faceLodSize;
	// These 2 are expressed in texels.
	ud->region->bufferRowLength = 0;
	ud->region->bufferImageHeight = 0;
	ud->region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ud->region->imageSubresource.mipLevel = miplevel;
	ud->region->imageSubresource.baseArrayLayer = face;
	ud->region->imageSubresource.layerCount = ud->numLayers * ud->numFaces;
	ud->region->imageOffset.x = 0;
	ud->region->imageOffset.y = 0;
	ud->region->imageOffset.z = 0;
	ud->region->imageExtent.width = width;
	ud->region->imageExtent.height = height;
	ud->region->imageExtent.depth = depth;

	ud->region ++;

	return KTX_SUCCESS;
}

namespace tt{
	ktx2::ktx2(const unsigned char* bytes, int size) {
		auto ret =ktxTexture2_CreateFromMemory(bytes,size,KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,(ktxTexture2**)&textureKtx);
		if(ret != KTX_SUCCESS){
			MY_LOG(ERROR)<<"ktx2 load:"<<ktxErrorString(ret);
			//throw
		}
		if(((ktxTexture2*)textureKtx)->isCompressed) {
			ret = ktxTexture2_TranscodeBasis((ktxTexture2*)textureKtx, KTX_TTF_ASTC_4x4_RGBA, 0);
			if(ret != KTX_SUCCESS){
				MY_LOG(ERROR)<<"ktx2 TranscodeBasis:"<<ktxErrorString(ret);
				//throw
			}
		}
	}

	ktx2::~ktx2() {
		ktxTexture_Destroy((ktxTexture*)textureKtx);
	}

	vk::ImageCreateInfo ktx2::vkImageCI() {
		auto This = (ktxTexture2*)textureKtx;
		vk::ImageType imageType;
		vk::ImageViewType viewType;
		switch (This->numDimensions) {
			case 1:
				imageType = vk::ImageType::e1D;
				viewType = This->isArray ?
				           vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
				break;
			case 2:
			default: // To keep compilers happy.
				imageType = vk::ImageType::e2D;
				if (This->isCubemap)
					viewType = This->isArray ?
					           vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
				else
					viewType = This->isArray ?
					           vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
				break;
			case 3:
				imageType = vk::ImageType::e3D;
				/* 3D array textures not supported in Vulkan. Attempts to create or
				 * load them should have been trapped long before this.
				 */
				assert(!This->isArray);
				viewType = vk::ImageViewType::e3D;
				break;
		}
		return vk::ImageCreateInfo{
				isCubemap() ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlagBits{},imageType,format(),
				{This->baseWidth,This->baseHeight,This->baseDepth},numLevels(),
				isCubemap() ? numLayers()*6 : numLayers(),vk::SampleCountFlagBits::e1,
				vk::ImageTiling::eOptimal,vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferDst,
				vk::SharingMode::eExclusive
		};
	}

	std::vector<vk::BufferImageCopy> ktx2::copyRegions() {
		std::vector<vk::BufferImageCopy> BufferImageCopys{((ktxTexture2*)textureKtx)->numLevels};
		user_cbdata_optimal cbData;
		cbData.region = reinterpret_cast<VkBufferImageCopy*>(BufferImageCopys.data());
		cbData.offset = 0;
		cbData.numFaces = ((ktxTexture2*)textureKtx)->numFaces;
		cbData.numLayers = numLayers();
		//cbData.dest = pMappedStagingBuffer;
		ktxTexture_IterateLevels((ktxTexture*)textureKtx,
		                         optimalTilingCallback,
		                         BufferImageCopys.data());
		return BufferImageCopys;
	}

	size_t ktx2::bufferSize() {
		return ((ktxTexture2*)textureKtx)->dataSize;
	}

	unsigned char *ktx2::bufferPtr() {
		return ((ktxTexture2*)textureKtx)->pData;
	}

	vk::Format ktx2::format() {
		return static_cast<vk::Format>(ktxTexture2_GetVkFormat((ktxTexture2*)textureKtx));
	}

	size_t ktx2::numLevels() {
		return ((ktxTexture2*)textureKtx)->numLevels;
	}

	bool ktx2::isCubemap() {
		return ((ktxTexture2*)textureKtx)->isCubemap;
	}

	size_t ktx2::numLayers() {
		return ((ktxTexture2*)textureKtx)->numLayers;
	}
}


