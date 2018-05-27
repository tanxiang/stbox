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
    protected:
        std::shared_ptr<Blob<Dtype>> top;
        std::shared_ptr<Blob<Dtype>> buttom;
    private:

    };

    template <typename Dtype>
    class FullConnect:public Connecter<Dtype>{
    public:
        FullConnect(){

        }
        Dtype forward();
        Dtype backward();

    };
    template <typename Dtype>
    class ConvConnect:public Connecter<Dtype>{
    public:
        ConvConnect(){

        }
        Dtype forward();
        Dtype backward();

    };
}
#endif //STBOX_CONNECT_HH
