#ifndef CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_AUTOTRADERAPPHANDLER_H
#define CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_AUTOTRADERAPPHANDLER_H

#include <memory>

#include <boost/asio/io_context.hpp>

#include "application.h"
#include "baseautotrader.h"
#include "connectivity.h"

namespace ReadyTraderGo {

class AutoTraderAppHandler
{
public:
    explicit AutoTraderAppHandler(Application& application, BaseAutoTrader& autoTrader)
        : mApplication(application), mAutoTrader(autoTrader), mContext(mApplication.GetContext())
    {
        mApplication.ConfigLoaded = [this](auto& tree) { ConfigLoadedHandler(tree); };
        mApplication.ReadyToRun = [this] { ReadyToRunHandler(); };
    }

private:
    void ConfigLoadedHandler(const boost::property_tree::ptree&);
    void ReadyToRunHandler();

    Application& mApplication;
    BaseAutoTrader& mAutoTrader;
    boost::asio::io_context& mContext;

    std::unique_ptr<ConnectionFactory> mExecConnectionFactory;
    std::unique_ptr<SubscriptionFactory> mInfoSubscriptionFactory;
};

}

#endif //CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_AUTOTRADERAPPHANDLER_H
