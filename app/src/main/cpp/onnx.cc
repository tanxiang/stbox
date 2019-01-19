//
// Created by ttand on 19-1-3.
//
#include "onnx.pb.h"
#include "onnx.hh"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <iostream>
#include <set>

struct FatKey   { int x; int data[1000]; };
struct LightKey { int x; };
//bool operator<(const FatKey& fk, const LightKey& lk) { return fk.x < lk.x; }
//bool operator<(const LightKey& lk, const FatKey& fk) { return lk.x < fk.x; }
bool operator<(const FatKey& fk1, const FatKey& fk2) { return fk1.x < fk2.x; }

namespace onnx {
    bool operator<(const onnx::ValueInfoProto &k1, const std::string &k2) { return k1.name() < k2; }

    bool operator<(const std::string &k1, const onnx::ValueInfoProto &k2) { return k1 < k2.name(); }

    bool operator<(const onnx::ValueInfoProto &k1, const onnx::ValueInfoProto &k2) {
        return k1.name() < k2.name();
    }
}



namespace tt {

    Onnx::Onnx(std::string fname) { //
        int fd = open(fname.c_str(), O_RDONLY | O_CLOEXEC);
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
            MY_LOG(INFO) << "Onnxfile opset_import " <<opset.domain();
        }
        if(modelProto.has_graph()){
            std::set<onnx::ValueInfoProto, std::less<>> inputMap{};
            auto& graph = modelProto.graph();

            for(auto&inputInfo:graph.input()){
                //MY_LOG(INFO) << "Onnx Input: " << input.name();
                //input.type().;
                inputMap.insert(inputInfo);
//                inputMap.emplace(inputInfo);
            }
            MY_LOG(INFO) << "Onnxfile graph " << graph.name() << graph.ByteSize() << "node:" << graph.node_size() << " tensor:" << graph.initializer_size();
            for(auto&outputInfo:graph.output()){
                MY_LOG(INFO) << "Onnx Output: " << outputInfo.name();
            }
            for(auto& initializer : graph.initializer()){
                //MY_LOG(INFO) << "Onnx Initializer: " << initializer.name();
            }

            for(auto& node : graph.node()){
                //MY_LOG(INFO) << "Onnx Node: " << node.name();
                node.output();
                for(auto&inputName:node.input()) {
//                    MY_LOG(INFO) << inputMap[inputName].DebugString();
                };
                node.op_type();
                node.attribute();
            }

            for(auto& value_info :graph.value_info()) {
                MY_LOG(INFO) << "Onnx value_info: " << value_info.name();
            }

        }
    }
}