#include <array>
#include <boost/asio/io_context.hpp>
#include <ready_trader_go/logging.h>
#include "autotrader.h"
using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

constexpr int LOT_SIZE = 10;
constexpr int POSITION_LIMIT = 100;
constexpr int UNHEDGED_POSITION_LIMIT = 10;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;

AutoTrader::AutoTrader(boost::asio::io_context& context) : BaseAutoTrader(context)
{
}

// logs
void AutoTrader::DisconnectHandler()
{
    BaseAutoTrader::DisconnectHandler();
    RLOG(LG_AT, LogLevel::LL_INFO) << "execution connection lost";
}

// calls OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
void AutoTrader::ErrorMessageHandler(unsigned long clientOrderId,
                                     const std::string& errorMessage)
{
    if (clientOrderId != 0 && ((mAsks.count(clientOrderId) == 1) || (mBids.count(clientOrderId) == 1)))
    {
        OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
    }
}

// cancel, insert, updates mPosition
void AutoTrader::OrderBookMessageHandler(Instrument instrument,
                                         unsigned long sequenceNumber,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    if (instrument == Instrument::FUTURE)
    {
        latestFutureAskPrice = askPrices[0];
        latestFutureBidPrice = bidPrices[0];
    }

    if (instrument == Instrument::ETF)
    {

        unsigned long newAskPrice = (askPrices[0] != 0) ? askPrices[0] - TICK_SIZE_IN_CENTS : 0;
        unsigned long newBidPrice = (bidPrices[0] != 0) ? bidPrices[0] + TICK_SIZE_IN_CENTS : 0;

        // cancel existing orders if the price has changed
        if (mAskId != 0 && newAskPrice != 0 && newAskPrice != mAskPrice)
        {
            SendCancelOrder(mAskId);
            mAskId = 0;
        }
        if (mBidId != 0 && newBidPrice != 0 && newBidPrice != mBidPrice)
        {
            SendCancelOrder(mBidId);
            mBidId = 0;
        }

        // insert new orders if the price has changed
        bool askOpp = latestFutureAskPrice < askPrices[0];
        bool bidOpp = latestFutureBidPrice > bidPrices[0];

        if (askOpp && mAskId == 0 && newAskPrice != 0 && mPosition > -POSITION_LIMIT && abs(mPosition - mPositionHedge) < UNHEDGED_POSITION_LIMIT)
        {
            mAskId = mNextMessageId++;
            mAskPrice = newAskPrice;
            SendInsertOrder(mAskId, Side::SELL, newAskPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
            mAsks.emplace(mAskId);
            mPosition -= (long)LOT_SIZE;
        }
        if (bidOpp && mBidId == 0 && newBidPrice != 0 && mPosition < POSITION_LIMIT && abs(mPosition - mPositionHedge) < UNHEDGED_POSITION_LIMIT)
        {
            mBidId = mNextMessageId++;
            mBidPrice = newBidPrice;
            SendInsertOrder(mBidId, Side::BUY, newBidPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
            mBids.emplace(mBidId);
            mPosition += (long)LOT_SIZE;
        }
    }
}

// sends hedge
void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    if (mAsks.count(clientOrderId) == 1)
    {
        mAskIdHedge = mNextMessageId++;
        SendHedgeOrder(mAskIdHedge, Side::BUY, MAX_ASK_NEAREST_TICK, volume);
        mAsksHedge.emplace(mAskIdHedge);
    }    
    else if (mBids.count(clientOrderId) == 1)
    {
        mBidIdHedge = mNextMessageId++;
        SendHedgeOrder(mBidIdHedge, Side::SELL, MIN_BID_NEARST_TICK, volume);
        mBidsHedge.emplace(mBidIdHedge);
    }
}


// updates mPositionHedge
void AutoTrader::HedgeFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    if (mAsksHedge.count(clientOrderId) == 1)
    {
        mPositionHedge -= volume;
        mAsksHedge.erase(clientOrderId);
    }    
    else if (mBidsHedge.count(clientOrderId) == 1)
    {
        mPositionHedge += volume;
        mBidsHedge.erase(clientOrderId);
    }
}

// updates mPosition
void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    if (remainingVolume == 0)
    {
        if (clientOrderId == mAskId)
        {
            mAskId = 0;
            mAsks.erase(clientOrderId);
            mPosition += LOT_SIZE - fillVolume;
        }
        else if (clientOrderId == mBidId)
        {
            mBidId = 0;
            mBids.erase(clientOrderId);
            mPosition -= LOT_SIZE - fillVolume;
        }
    }
}

void AutoTrader::TradeTicksMessageHandler(Instrument instrument,
                                          unsigned long sequenceNumber,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
}