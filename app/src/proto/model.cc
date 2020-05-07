//
// Created by ttand on 20-4-3.
//

#include "model.hh"
#include "model.pb.h"

namespace tt{
	unsigned int AssetFdBuildDevMemory(int fd, int alt){
		google::protobuf::SetLogHandler([](google::protobuf::LogLevel level, const char* filename, int line, const std::string& message)
		                                {
			                                MY_LOG(ERROR) << "message:::" << message ;
		                                });
		if (fd >= 0) {
			ptfile::Model model;
			if(!model.ParseFromFileDescriptor(fd))
				MY_LOG(ERROR)<<"ParseFromFileDescriptor:::false:"<<fd;

			MY_LOG(INFO)<<"loadmodule:::"<<model.name();
			for(auto& mesh:model.meshs()){
				MY_LOG(INFO)<<"mesh:::"<<mesh.name();
			}
		}
		return 0;
	}
}
