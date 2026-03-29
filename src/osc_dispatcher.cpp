#include "osc_dispatcher.h"

#define MAX_ROUTES 16

struct OscRoute
{
    const char* prefix;
    OscHandler handler;
};

static OscRoute routes[MAX_ROUTES];
static int routeCount = 0;

static void addRoute(const char* prefix, OscHandler handler)
{
    if (routeCount >= MAX_ROUTES)
        return;

    routes[routeCount].prefix = prefix;
    routes[routeCount].handler = handler;
    routeCount++;
}

void oscRegisterRoutes()
{
    routeCount = 0;

    addRoute("/ch/", xairHandleChannel);
    addRoute("/headamp/", xairHandleHeadamp);
    addRoute("/bus/", xairHandleBus);
    addRoute("/lr/", xairHandleMain);
    addRoute("/config/", xairHandleConfig);
    addRoute("/meters", xairHandleMeters);
    addRoute("/xinfo", xairHandleGlobal);
    addRoute("/status", xairHandleGlobal);
    

}

void oscDispatch(OSCMessage& msg, char address[OSC_BUFFER_SIZE])
{
    DBG2("Dispatching OSC Address:", address);

    for (int i = 0; i < routeCount; i++)
    {
        const char* prefix = routes[i].prefix;
        size_t len = strlen(prefix);

        if (strncmp(address, prefix, len) == 0)
        {
            //  Wenn Prefix NICHT mit '/' endet → Boundary prüfen
            if (prefix[len - 1] != '/')
            {
                if (!(address[len] == '/' || address[len] == '\0'))
                    continue;
            }

            DBG2("Matched route:", prefix);
            routes[i].handler(msg, address);
            return;
        }
    }

    DBG("No OSC route matched!");
}