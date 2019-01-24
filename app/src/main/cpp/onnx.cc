//
// Created by ttand on 19-1-3.
//
#include "onnx.pb.h"
#include "onnx.hh"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <iostream>
#include <set>


namespace onnx {

    bool operator<(const ValueInfoProto &k1, const std::string &k2) { return k1.name() < k2; }
    bool operator<(const std::string &k1, const ValueInfoProto &k2) { return k1 < k2.name(); }
    bool operator<(const NodeProto &k1, const std::string &k2) { return k1.name() < k2; }
    bool operator<(const std::string &k1, const NodeProto &k2) { return k1 < k2.name(); }
    template <typename T>
    bool operator<(const T &k1, const T &k2) {
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
            std::set<onnx::ValueInfoProto,std::less<>> inputSet{};
            std::set<onnx::ValueInfoProto,std::less<>> outputSet{};
            std::set<onnx::NodeProto,std::less<>> nodeSet{};
            std::map<std::string,std::string> nodeOutputMap{};

            auto& graph = modelProto.graph();

            for(auto&inputInfo:graph.input()){
                //MY_LOG(INFO) << "Onnx Input: " << input.name();
                //input.type().;
                inputSet.insert(inputInfo);
            }
            for(auto&outputInfo:graph.output()){
                //MY_LOG(INFO) << "Onnx Output: " << outputInfo.name();
                outputSet.insert(outputInfo);
            }
            for(auto& initializer : graph.initializer()){
                //MY_LOG(INFO) << "Onnx Initializer: " << initializer.name();
            }


            for(auto& node : graph.node()){
                //MY_LOG(INFO) << "Onnx Node: " << node.name();

                for(auto&inputName:node.input()) {
//                    MY_LOG(INFO) << inputSet[inputName].DebugString();
                    auto inputInfoItr = inputSet.find(inputName);
                    if(inputInfoItr == inputSet.end()) {
                        if(auto nodeOutputitr = nodeOutputMap.find(inputName) == nodeOutputMap.end()){
                            MY_LOG(ERROR) << "node " << node.name() << "'s input " << inputName << " not found!!";
                        }
                    }
                };
                for(auto&outputName:node.output()){
                    auto outputIfoItr = outputSet.find(outputName);
                    if(outputIfoItr == outputSet.end()){
                        nodeOutputMap.emplace(outputName,node.name());
                        //MY_LOG(ERROR) << "node " << node.name() << "'s output " << outputName
                        //              << " not found!!";
                    }
                }
                node.op_type();
                node.attribute();
                nodeSet.insert(node);
            }

            for(auto& value_info :graph.value_info()) {
                MY_LOG(INFO) << "Onnx value_info: " << value_info.name();
            }

        }
    }
}