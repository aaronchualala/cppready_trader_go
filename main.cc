#include <cstdlib>
#include <iostream>

#include <ready_trader_go/application.h>
#include <ready_trader_go/autotraderapphandler.h>
#include <ready_trader_go/error.h>

#include "autotrader.h"

int main(int argc, char* argv[])
{
    try
    {
        ReadyTraderGo::Application app;
        AutoTrader trader{app.GetContext()};
        ReadyTraderGo::AutoTraderAppHandler appHandler{app, trader};
        app.Run(argc, argv);
    }
    catch (const ReadyTraderGo::ReadyTraderGoError& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        // Catch block added so the Application object gets destructed
        // and the log gets flushed.
        throw;
    }

    return EXIT_SUCCESS;
}
