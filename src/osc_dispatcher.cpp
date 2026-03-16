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
    addRoute("/bus/", xairHandleBus);
    addRoute("/main/", xairHandleMain);
    addRoute("/meters", xairHandleMeters);
}

void oscDispatch(OSCMessage& msg)
{
    char address[64];

    msg.getAddress(address, sizeof(address));

    for (int i = 0; i < routeCount; i++)
    {
        if (strncmp(address, routes[i].prefix, strlen(routes[i].prefix)) == 0)
        {
            routes[i].handler(msg, address);
            return;
        }
    }
}