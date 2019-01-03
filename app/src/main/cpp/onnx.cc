//
// Created by ttand on 19-1-3.
//
#include "onnx.pb.h"
#include "onnx.hh"
#include <google/protobuf/io/zero_copy_stream_impl.h>

Onnx::Onnx() {
    onnx::ModelProto modelProto;
    google::protobuf::io::FileInputStream fileInputStream{3};
    modelProto.ParseFromZeroCopyStream(&fileInputStream);
}