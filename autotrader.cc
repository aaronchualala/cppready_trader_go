 /* 
rm build/autotrader
rm autotrader
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build --config Debug
cp build/autotrader .
python3 rtg.py run autotrader 
*/
#include <array>
#include <boost/asio/io_context.hpp>
#include <ready_trader_go/logging.h>
#include "autotrader.h"
using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

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

// OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
void AutoTrader::ErrorMessageHandler(unsigned long clientOrderId,
                                     const std::string& errorMessage)
{
    if (clientOrderId != 0 && ((mAsks.count(clientOrderId) == 1) || (mBids.count(clientOrderId) == 1)))
    {
        OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
    }
}

// TO UPDATE - inserts
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
    if (instrument == Instrument::ETF){
        prevLowestAsk = askPrices[0];
        prevHighestBid = bidPrices[0];
    }
    
    
    if (instrument == Instrument::FUTURE){
        if (askPrices[0] < prevLowestAsk){
            RLOG(LG_AT, LogLevel::LL_INFO) << "FRIDGE: " << (myEtfHoldings - (myEtfAskPosition+myVolume) + myEtfBidPosition); 
            if(mAsks.size() == 0 && myEtfHoldings - (myEtfAskPosition+myVolume) > -POSITION_LIMIT){
                id = nextMessageId++;
                myPrice = prevLowestAsk - TICK_SIZE_IN_CENTS;
                SendInsertOrder(id, Side::SELL, myPrice, myVolume, Lifespan::GOOD_FOR_DAY);
                mAsks.emplace(id);
                myEtfAskPosition += myVolume;
            }
        } else {
            if (mAsks.size() != 0){
                id = *mAsks.begin();
                SendCancelOrder(id);
            }
        }

        if (bidPrices[0] > prevHighestBid){
            if(mBids.size() == 0 && myEtfHoldings + (myEtfBidPosition+myVolume) < POSITION_LIMIT){
                id = nextMessageId++;
                myPrice = prevHighestBid + TICK_SIZE_IN_CENTS;
                SendInsertOrder(id, Side::BUY, myPrice, myVolume, Lifespan::GOOD_FOR_DAY);
                mBids.emplace(id);
                myEtfBidPosition += myVolume;
            }
        } else {
            if (mBids.size() != 0){
                id = *mBids.begin();
                SendCancelOrder(id);
            }
        }
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";   
    RLOG(LG_AT, LogLevel::LL_INFO) << "myEtfHoldings: " << myEtfHoldings << "| myEtfAskPosition: " << myEtfAskPosition << "| myEtfBidPosition: " << myEtfBidPosition ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAsks: " << mAsks.size() << "| mBids: " << mBids.size() << "| prevLowestAsk: " << prevLowestAsk << "| prevHighestBid: " << prevHighestBid << "| myFutureHoldings: " << myFutureHoldings ;


}

// TO UPDATE - fills
void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "OrderFilledMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    if (mAsks.count(clientOrderId) == 1)
    {
        myEtfHoldings -= volume;
        myEtfAskPosition -= volume;
        SendHedgeOrder(nextMessageId++, Side::BUY, MAX_ASK_NEAREST_TICK, volume);
        myFutureHoldings += volume;
    }
    else if (mBids.count(clientOrderId) == 1)
    {
        myEtfHoldings += volume;
        myEtfBidPosition -= volume;
        SendHedgeOrder(nextMessageId++, Side::SELL, MIN_BID_NEARST_TICK, volume);
        myFutureHoldings -= volume;
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "myEtfHoldings: " << myEtfHoldings << "| myEtfAskPosition: " << myEtfAskPosition << "| myEtfBidPosition: " << myEtfBidPosition ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAsks: " << mAsks.size() << "| mBids: " << mBids.size() << "| prevLowestAsk: " << prevLowestAsk << "| prevHighestBid: " << prevHighestBid << "| myFutureHoldings: " << myFutureHoldings ;
}


// NIL
void AutoTrader::HedgeFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
}

// TO UPDATE - cancels
void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "\n\n";
    RLOG(LG_AT, LogLevel::LL_INFO) << "OrderStatusMessageHandler";
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    if (mAsks.count(clientOrderId) == 1)
    {
        if (remainingVolume == 0)
        {
            mAsks.erase(clientOrderId);
            myEtfAskPosition -= (myVolume - fillVolume);
        }
    }
    else if (mBids.count(clientOrderId) == 1)
    {
        if (remainingVolume == 0)
        {
            mBids.erase(clientOrderId);
            myEtfBidPosition -= (myVolume - fillVolume);
        }
    }
    RLOG(LG_AT, LogLevel::LL_INFO) << "————————————————————————————————————————————————————————";
    RLOG(LG_AT, LogLevel::LL_INFO) << "myEtfHoldings: " << myEtfHoldings << "| myEtfAskPosition: " << myEtfAskPosition << "| myEtfBidPosition: " << myEtfBidPosition ;
    RLOG(LG_AT, LogLevel::LL_INFO) << "mAsks: " << mAsks.size() << "| mBids: " << mBids.size() << "| prevLowestAsk: " << prevLowestAsk << "| prevHighestBid: " << prevHighestBid << "| myFutureHoldings: " << myFutureHoldings ;
}

// NIL
void AutoTrader::TradeTicksMessageHandler(Instrument instrument,
                                          unsigned long sequenceNumber,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
}