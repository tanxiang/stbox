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
            std::cerr <<"opening :"<< fname << strerror(errno) ;
            return;
        }
        onnx::ModelProto modelProto;
        auto fileInputStream = std::make_unique<google::protobuf::io::FileInputStream>(fd);
        fileInputStream->SetCloseOnDelete(true);
        modelProto.ParseFromZeroCopyStream(fileInputStream.get());
    }
}