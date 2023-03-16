// cmake -DCMAKE_BUILD_TYPE=Debug -B build
// cmake --build build --config Debug
// cp build/autotrader .
// python3 rtg.py run autotrader 
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
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "ErrorMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
}

// cancel, insert, updates mPosition
void AutoTrader::OrderBookMessageHandler(Instrument instrument,
                                         unsigned long sequenceNumber,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "OrderBookMessageHandler";       
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————"; 
    if (instrument == Instrument::FUTURE)
    {
        latestFutureAskPrice = askPrices[0];
        latestFutureBidPrice = bidPrices[0];
    }

    if (instrument == Instrument::ETF)
    {

        unsigned long newAskPrice = (askPrices[0] != 0 && askPrices[0] != mAskPrice && askVolumes[0] != LOT_SIZE) ? askPrices[0] - TICK_SIZE_IN_CENTS : 0;
        unsigned long newBidPrice = (bidPrices[0] != 0 && bidPrices[0] != mBidPrice && bidVolumes[0] != LOT_SIZE) ? bidPrices[0] + TICK_SIZE_IN_CENTS : 0;

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

        if (askOpp && mAskId == 0 && newAskPrice != 0 && mPosition - 10 > -POSITION_LIMIT && abs(mPosition - mPositionHedge) <= UNHEDGED_POSITION_LIMIT)
        {
            mAskId = mNextMessageId++;
            mAskPrice = newAskPrice;
            SendInsertOrder(mAskId, Side::SELL, newAskPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
            mAsks.emplace(mAskId);
            mPosition -= LOT_SIZE;
        }

        if (bidOpp && mBidId == 0 && newBidPrice != 0 && mPosition + 10 < POSITION_LIMIT && abs(mPosition - mPositionHedge) <= UNHEDGED_POSITION_LIMIT)
        {
            mBidId = mNextMessageId++;
            mBidPrice = newBidPrice;
            SendInsertOrder(mBidId, Side::BUY, newBidPrice, LOT_SIZE, Lifespan::GOOD_FOR_DAY);
            mBids.emplace(mBidId);
            mPosition += LOT_SIZE;
        }
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";   
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
}

// sends hedge
void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "OrderFilledMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    if (mAsks.count(clientOrderId) == 1)
    {
        mBidIdHedge = mNextMessageId++;
        SendHedgeOrder(mBidIdHedge, Side::BUY, MAX_ASK_NEAREST_TICK, volume);
        mBidsHedge.emplace(mBidIdHedge);
    }    
    else if (mBids.count(clientOrderId) == 1)
    {
        mAskIdHedge = mNextMessageId++;
        SendHedgeOrder(mAskIdHedge, Side::SELL, MIN_BID_NEARST_TICK, volume);
        mAsksHedge.emplace(mAskIdHedge);
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
}


// updates mPositionHedge
void AutoTrader::HedgeFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "HedgeFilledMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";

    if (mAsksHedge.count(clientOrderId) == 1)
    {
        mPositionHedge += volume;
        mAsksHedge.erase(clientOrderId);
    }    
    else if (mBidsHedge.count(clientOrderId) == 1)
    {
        mPositionHedge -= volume;
        mBidsHedge.erase(clientOrderId);
    }

    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
}

// updates mPosition
void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "OrderStatusMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    if (remainingVolume == 0)
    {
        if (mAsks.count(clientOrderId) == 1)
        {
            if(mAskId == clientOrderId)
            {
                mAskId = 0;
            }
            mAsks.erase(clientOrderId);
            mPosition += LOT_SIZE - fillVolume;
        }
        else if (mBids.count(clientOrderId) == 1)
        {
           if(mBidId == clientOrderId)
            {
                mBidId = 0;
            }
            mBids.erase(clientOrderId);
            mPosition -= LOT_SIZE - fillVolume;
        }
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
}

void AutoTrader::TradeTicksMessageHandler(Instrument instrument,
                                          unsigned long sequenceNumber,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "TradeTicksMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";

    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "mPosition: " << mPosition << "| mPositionHedge: " << mPositionHedge ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAskId: " << mAskId << "| mBidId: " << mBidId ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";

}