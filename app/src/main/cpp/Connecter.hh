//
// Created by ttand on 18-5-27.
//

#ifndef STBOX_CONNECT_HH
#define STBOX_CONNECT_HH

#include "Blob.hh"
namespace tt {
    template <typename Dtype>
    class Connecter{
    public:
        Connecter(){

        };
        Dtype forward();
        Dtype backward();
    protected:
        std::shared_ptr<Blob<Dtype>> top;
        std::shared_ptr<Blob<Dtype>> buttom;
    private:

    };

    template <typename Dtype>
    class FullConnect:public Connecter{

    };
}
#endif //STBOX_CONNECT_HH
