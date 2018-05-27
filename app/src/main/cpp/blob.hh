//
// Created by ttand on 18-5-25.
//

#ifndef STBOX_BLOB_HH
#define STBOX_BLOB_HH


#include "util.hh"

namespace tt {
    template <typename Dtype>
    class Blob{

    public:
        explicit Blob(Device device,int num,int channelNum,int height,int width){
            shape[0] = num;
            shape[1] = channelNum;
            shape[2] = height;
            shape[3] = width;
        }

    private:
        std::array<int,4> shape;
        std::unique_ptr<Dtype[]> data;

    };
}

#endif //STBOX_BLOB_HH
