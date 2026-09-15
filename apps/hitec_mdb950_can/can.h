#pragma once

#include <stdint.h>

struct TCanFrame
{
    uint32_t id;
    uint8_t length;
    uint8_t extended;
    uint8_t remote;
    uint8_t data[8];
};

class CCan
{
public:
    bool Begin(uint32_t baud);
    void End();
    bool Transmit(const TCanFrame& frame);
    bool Receive(TCanFrame& frame);
    uint32_t ErrorStatus() const;
};
