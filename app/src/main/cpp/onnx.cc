//
// Created by ttand on 19-1-3.
//
#include "onnx.pb.h"
#include "onnx.hh"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <iostream>

namespace tt {
    Onnx::Onnx(std::string fname) { //
        int fd = open(fname.c_str(), O_RDONLY);
        if (fd < 0) {
            MY_LOG(ERROR) <<"opening :"<< fname << strerror(errno) ;
            return;
        }
        onnx::ModelProto modelProto;
        auto fileInputStream = std::make_unique<google::protobuf::io::FileInputStream>(fd);
        fileInputStream->SetCloseOnDelete(true);
        modelProto.ParseFromZeroCopyStream(fileInputStream.get());
        MY_LOG(INFO) << "Onnxfile length:" << modelProto.ByteSize() ;
        for(auto& opset:modelProto.opset_import()){
            MY_LOG(INFO) << "Onnxfile opset_import" <<opset.DebugString();
        }
        if(modelProto.has_graph()){
            auto graph = modelProto.graph();
            MY_LOG(INFO) << "Onnxfile graph " << graph.name() << graph.ByteSize() << "node:" << graph.node_size() << " tensor:" << graph.initializer_size();
            graph.node();
        }
    }
}