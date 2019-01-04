//
// Created by ttand on 19-1-3.
//
#include "onnx.pb.h"
#include "onnx.hh"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <iostream>

using google::protobuf::io::FileInputStream;
Onnx::Onnx(std::string fname) { ///storage/0123-4567/nw/mobilenetv2-1.0.onnx
    int fd = open(fname.c_str(), O_RDONLY);
    if(fd < 0) {
        std::cerr << strerror(errno) << fname;
        return;
    }
    onnx::ModelProto modelProto;
    auto fileInputStream = std::make_unique<FileInputStream>(fd);
    fileInputStream->SetCloseOnDelete(true);
    modelProto.ParseFromZeroCopyStream(fileInputStream.get());
}