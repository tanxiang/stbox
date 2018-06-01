//
// Created by ttand on 18-5-27.
//

#ifndef STBOX_CONNECT_HH
#define STBOX_CONNECT_HH

#include "Blob.hh"
#include <set>
namespace tt {
    template <typename Dtype>
    class Connecter{
    public:
        Connecter(){

        };
        virtual Dtype forward()=0;
        virtual Dtype backward()=0;
    protected:
        std::shared_ptr<Blob<Dtype>> top;
        std::shared_ptr<Blob<Dtype>> buttom;
    private:
        std::set<Connecter<Dtype>> forwardLink;
        std::set<Connecter<Dtype>*> backwardLink;
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
