//
// Created by ttand on 18-5-27.
//

#ifndef STBOX_CONNECT_HH
#define STBOX_CONNECT_HH

#include "blob.hh"
namespace tt {
    template <typename Dtype>
    class connect{
    public:
        Dtype forward();
        Dtype backward();
    protected:
    private:
        std::shared_ptr<Blob<Dtype>> top;
        std::shared_ptr<Blob<Dtype>> buttom;
    };
}
#endif //STBOX_CONNECT_HH
