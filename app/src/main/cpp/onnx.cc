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
    bool operator<(const ValueInfoProto &k1, const ValueInfoProto &k2) {
        return k1.name() < k2.name();
    }

    bool operator<(const TensorProto &k1, const std::string &k2) { return k1.name() < k2; }
    bool operator<(const std::string &k1, const TensorProto &k2) { return k1 < k2.name(); }
    bool operator<(const TensorProto &k1, const TensorProto &k2) {
        return k1.name() < k2.name();
    }

    bool operator<(const NodeProto &k1, const std::string &k2) { return (k1.has_name()?k1.name():k1.output(0)) < k2; }
    bool operator<(const std::string &k1, const NodeProto &k2) { return k1 < (k2.has_name()?k2.name():k2.output(0)); }
    bool operator<(const NodeProto &k1, const NodeProto &k2) {
        return (k1.has_name()?k1.name():k1.output(0)) < (k2.has_name()?k2.name():k2.output(0));
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

        //for(auto& opset:modelProto.opset_import()){
        //    MY_LOG(INFO) << "Onnxfile opset_import " <<opset.kDomainFieldNumber;
        //}
        if(modelProto.has_graph()){
            std::set<onnx::ValueInfoProto,std::less<>> inputSet{};
            std::set<onnx::ValueInfoProto,std::less<>> outputSet{};
            std::set<onnx::NodeProto,std::less<>> nodeSet{};
            std::set<onnx::TensorProto,std::less<>> tensorSet{};

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

            for(auto& node : graph.node()){
                //MY_LOG(INFO) << "Onnx Node: " << node.name();
                nodeSet.insert(node);
            }
            for(auto& initializer : graph.initializer()){
                //MY_LOG(INFO) << "Onnx Initializer: " << initializer.name();
                tensorSet.insert(initializer);
            }

            std::multimap<std::string,std::string> outputNodeMap{},inputNodeMap{};

            for(auto& node:nodeSet){
                for(auto&inputName:node.input()) {
                    inputNodeMap.emplace(inputName,node.has_name()?node.name():node.output(0));
                };
                if(node.has_name()) {
                    for (auto &outputName:node.output()) {
                        outputNodeMap.emplace(outputName, node.name());
                    }
                }
            }
            for(auto& value_info :graph.value_info()) {
                MY_LOG(INFO) << "Onnx value_info: " << value_info.name();
            }

            //fw test
            std::string firstInputName;
            for(auto&[inputName,nodeName]:inputNodeMap){
                if(tensorSet.find(inputName)==tensorSet.end() &&
                outputNodeMap.find(inputName)==outputNodeMap.end() &&
                nodeSet.find(inputName) == nodeSet.end()){
                    MY_LOG(INFO) << inputName <<" in "<< nodeName <<":input first node!";
                    firstInputName = inputName;
                }
            }

            if(!firstInputName.empty()){
                auto nodeRange = inputNodeMap.equal_range(firstInputName);
                for(auto nodeItr = nodeRange.first; nodeItr != nodeRange.second;++nodeItr){
                    MY_LOG(INFO) << nodeItr->second;
                }
                //auto nodeItr = nodeSet.find(firstInputName);
                //for(auto&inputName : nodeItr->input()){
                //    MY_LOG(INFO) << "";
                //}
            }
        }
    }
}