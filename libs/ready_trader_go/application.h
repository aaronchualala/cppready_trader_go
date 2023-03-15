#ifndef CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_APPLICATION_H
#define CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_APPLICATION_H

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/sinks/drop_on_overflow.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>

namespace ReadyTraderGo {

constexpr std::size_t LOG_QUEUE_SIZE = 1024;

class Application
{
public:
    Application() : mContext(), mName(), mSignals(mContext) {}
    ~Application();

    // Application instances can't be copied or moved
    Application(const Application&) = delete;
    void operator=(const Application&) = delete;
    Application(Application&& other) = delete;
    void operator=(Application&& other) = delete;

    boost::asio::io_context& GetContext() { return mContext; }

    void Run(int argc, char* argv[]);

    std::function<void(const boost::property_tree::ptree&)> ConfigLoaded;
    std::function<void()> ReadyToRun;

private:
    void OnConfigLoaded(const boost::property_tree::ptree& tree) const;
    void OnReadyToRun() const;

    void LoadConfig(const std::string& filename);
    void SetUpLogging();
    void SignalHandler(const boost::system::error_code& error, int signal);
    void TearDownLogging();

    boost::asio::io_context mContext;
    std::string mName;
    boost::asio::signal_set mSignals;

    using sink_t = boost::log::sinks::asynchronous_sink<
        boost::log::sinks::text_ostream_backend,
        boost::log::sinks::bounded_fifo_queue<LOG_QUEUE_SIZE, boost::log::sinks::drop_on_overflow>>;
    boost::shared_ptr<sink_t> mSink;
};

inline void Application::OnConfigLoaded(const boost::property_tree::ptree& tree) const
{
    if (ConfigLoaded)
    {
        ConfigLoaded(tree);
    }
}

inline void Application::OnReadyToRun() const
{
    if (ReadyToRun)
    {
        ReadyToRun();
    }
}

}

#endif //CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_APPLICATION_H
