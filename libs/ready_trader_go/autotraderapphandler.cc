#include <memory>

#include <boost/property_tree/ptree.hpp>

#include "autotraderapphandler.h"
#include "connectivity.h"
#include "config.h"
#include "error.h"

namespace ReadyTraderGo {

void AutoTraderAppHandler::ConfigLoadedHandler(const boost::property_tree::ptree& tree)
{
    Config config;
    config.readFromPropertyTree(tree);

    if (config.mTeamName.size() > MessageFieldSize::STRING)
        throw ReadyTraderGoError("configured team name is too long");

    if (config.mSecret.size() > MessageFieldSize::STRING)
        throw ReadyTraderGoError("configured secret is too long");

    mExecConnectionFactory = std::make_unique<ConnectionFactory>(mContext,
                                                                 config.mExecHost,
                                                                 config.mExecPort);
    mInfoSubscriptionFactory = std::make_unique<SubscriptionFactory>(mContext,
                                                                     config.mInfoType,
                                                                     config.mInfoName);

    mAutoTrader.SetLoginDetails(config.mTeamName, config.mSecret);
}

void AutoTraderAppHandler::ReadyToRunHandler()
{
    auto connection = mExecConnectionFactory->Create();
    mAutoTrader.SetExecutionConnection(std::move(connection));
    auto subscription = mInfoSubscriptionFactory->Create();
    mAutoTrader.SetInformationSubscription(std::move(subscription));
}

}
