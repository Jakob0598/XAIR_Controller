#pragma once

#include "config.h"

// ==============================
// API
// ==============================

void xairRequestStart();
void xairRequestProcess();
bool xairRequestRunning();

// ==============================
// SINGLE REQUESTS (non-blocking)
// ==============================

void xairRequestChannel(uint8_t ch);
void xairRequestSends(uint8_t ch);
void xairRequestEq(uint8_t ch);
void xairRequestChannelFull(uint8_t ch);

void xairRequestBus(uint8_t bus);
void xairRequestMain();
void xairRequestMeters();