//
// Created by ttand on 20-4-3.
//

#include "model.hh"
#include "model.pb.h"

namespace tt{
	unsigned int AssetBuildDevMemory(android_app *app, AAssetHander &assetHander , int alt){
		off_t start,length;
		const int fd = AAsset_openFileDescriptor(assetHander.get(), &start, &length);
		if (fd >= 0) {
			ptfile::Model model;
			model.ParseFromFileDescriptor(fd);
			MY_LOG(INFO)<<model.name();
			for(auto& mesh:model.meshs()){
				MY_LOG(INFO)<<mesh.name();

			}
		}
		return 0;
	}
}
