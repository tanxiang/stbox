//
// Created by ttand on 18-5-27.
//

#ifndef STBOX_CONNECT_HH
#define STBOX_CONNECT_HH

#include "blob.hh"
namespace tt {
    template <typename Dtype>
    class connecter{
    public:
        connecter(){

        };
        Dtype forward();
        Dtype backward();
    protected:
        std::shared_ptr<Blob<Dtype>> top;
        std::shared_ptr<Blob<Dtype>> buttom;
    private:

    };
}
#endif //STBOX_CONNECT_HH
