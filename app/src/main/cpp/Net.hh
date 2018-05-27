//
// Created by ttand on 18-5-27.
//

#ifndef STBOX_NET_HH
#define STBOX_NET_HH

#include "Connecter.hh"
#include "Activation.hh"
#include <list>

namespace tt{
    template <typename Dtype>
    class Net{
    private:
        std::list<Connecter> data;
    };

}
#endif //STBOX_NET_HH
