#ifndef CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONFIG_H
#define CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONFIG_H

#include <string>

#include <boost/property_tree/ptree.hpp>

namespace ReadyTraderGo {

struct Config
{
    void readFromPropertyTree(const boost::property_tree::ptree& tree)
    {
        mExecHost = tree.get<std::string>("Execution.Host");
        mExecPort = tree.get<unsigned short>("Execution.Port");

        mInfoType = tree.get<std::string>("Information.Type");
        mInfoName = tree.get<std::string>("Information.Name");

        mTeamName = tree.get<std::string>("TeamName");
        mSecret = tree.get<std::string>("Secret");
    }

    std::string mExecHost;
    unsigned short mExecPort;

    std::string mInfoType;
    std::string mInfoName;

    std::string mTeamName;
    std::string mSecret;
};

}

#endif //CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONFIG_H
