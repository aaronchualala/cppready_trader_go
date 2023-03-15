#ifndef CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_ERROR_H
#define CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_ERROR_H

#include <stdexcept>

namespace ReadyTraderGo {

struct ReadyTraderGoError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}

#endif //CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_ERROR_H
