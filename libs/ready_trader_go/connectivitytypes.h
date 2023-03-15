#ifndef CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONNECTIVITYTYPES_H
#define CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONNECTIVITYTYPES_H

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace ReadyTraderGo {

enum class SendMode
{
    ASAP,
    SOON
};

struct ISerialisable
{
    virtual std::size_t Size() const noexcept = 0;
    virtual void Deserialise(unsigned char const*, std::size_t) = 0;
    virtual void Serialise(unsigned char*) const = 0;
};

struct IConnection
{
    virtual ~IConnection() = default;
    virtual void AsyncRead() = 0;
    virtual void SendMessage(unsigned char messageType,
                             const ISerialisable& serialisable,
                             SendMode mode) = 0;
    void SendMessage(unsigned char messageType, const ISerialisable& serialisable)
    {
        SendMessage(messageType, serialisable, SendMode::ASAP);
    }

    const std::string& GetName() const { return mName; }
    void SetName(std::string name) { mName = std::move(name); }

    std::function<void()> Disconnected;
    std::function<void(IConnection*, unsigned char, unsigned char const*, std::size_t)> MessageReceived;

protected:
    void OnDisconnect()
    {
        if (Disconnected)
        {
            Disconnected();
        }
    }

    void OnMessageReceipt(unsigned char messageType, unsigned char const* data, std::size_t size)
    {
        if (MessageReceived)
        {
            MessageReceived(this, messageType, data, size);
        }
    }

    std::string mName;
};

struct ISubscription: public std::enable_shared_from_this<ISubscription>
{
    virtual ~ISubscription() = default;
    virtual void AsyncReceive() = 0;

    const std::string& GetName() const { return mName; }
    void SetName(std::string name) { mName = std::move(name); }

    std::function<void(ISubscription*, unsigned char, unsigned char const*, std::size_t)> MessageReceived;

protected:
    void OnMessageReceipt(unsigned char messageType, unsigned char const* data, std::size_t size)
    {
        if (MessageReceived)
        {
            MessageReceived(this, messageType, data, size);
        }
    }

    std::string mName;
};

struct IConnectionFactory
{
    virtual ~IConnectionFactory() = default;
    virtual std::unique_ptr<IConnection> Create() = 0;
};

struct ISubscriptionFactory
{
    virtual ~ISubscriptionFactory() = default;
    virtual std::shared_ptr<ISubscription> Create() = 0;
};

}

#endif //CPPREADY_TRADER_GO_LIBS_READY_TRADER_GO_CONNECTIVITYTYPES_H
