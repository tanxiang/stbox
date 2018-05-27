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
        int num() const {return shape[0];}
        int channelNum() const {return shape[1];}
        int height() const {return shape[2];}
        int width() const {return shape[3];}
        int offset(const int n, const int c = 0, const int h = 0,
                             const int w = 0) const {
            return ((n * channelNum() + c) * height() + h) * width() + w;
        }
    private:
        std::array<int,4> shape;
        std::unique_ptr<Dtype[]> data;

    };
}

#endif //STBOX_BLOB_HH
