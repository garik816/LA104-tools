#include <library.h>
#include "can.h"

using namespace BIOS;

namespace
{
const uint32_t kBauds[] = {125000, 250000, 500000, 1000000};
const int kBaudCount = sizeof(kBauds) / sizeof(kBauds[0]);

enum EProtocol { ProtocolNone, ProtocolHitec, ProtocolDroneCan, ProtocolCanOpen, ProtocolJ1939, ProtocolUnknown };

CCan gCan;
uint32_t gFrames = 0;
uint32_t gBaudStart = 0;
int gBaudIndex = 0;
bool gAuto = true;
bool gOnline = false;
int gHitec = 0;
int gDrone = 0;
int gOpen = 0;
int gJ1939 = 0;
int gExtended = 0;
int gStandard = 0;
uint32_t gLastId = 0;
bool gLastExtended = false;
uint8_t gLastData[8];
uint8_t gLastLength = 0;

const char* BaudName(uint32_t baud)
{
    switch (baud) { case 125000: return "125k"; case 250000: return "250k"; case 500000: return "500k"; case 1000000: return "1M"; default: return "?"; }
}

bool IsHitecMessage(const TCanFrame& f)
{
    if (!f.length) return false;
    uint8_t m = f.data[0];
    return (m == 'r' && f.length == 3) || (m == 'v' && f.length == 5) ||
        (m == 'R' && f.length == 4) || (m == 'V' && f.length == 8) ||
        ((m == 'w' || m == 'x') && f.length == 5) || ((m == 'W' || m == 'X') && f.length == 8);
}

bool IsDroneCanFrame(const TCanFrame& f)
{
    if (!f.extended || !f.length) return false;
    uint8_t tail = f.data[f.length - 1];
    uint8_t transport = tail & 0xe0;
    if (transport != 0xc0 && transport != 0x80 && transport != 0x40 && transport != 0x20 && transport != 0x00)
        return false;
    uint8_t source = f.id & 0x7f;
    bool service = (f.id & 0x80) != 0;
    if (!source) return false;
    if (service && (((f.id >> 8) & 0x7f) == 0)) return false;
    return true;
}

bool IsCanOpenSdo(uint8_t value)
{
    return value == 0x40 || value == 0x43 || value == 0x4b || value == 0x4f || value == 0x60 || value == 0x80;
}

void Classify(const TCanFrame& f)
{
    if (f.extended) gExtended++; else gStandard++;
    if (IsHitecMessage(f)) gHitec += 5;
    if (IsDroneCanFrame(f))
    {
        gDrone++;
        uint16_t type = (uint16_t)((f.id >> 8) & 0xffff);
        if (!(f.id & 0x80) && type == 341) gDrone += 8; // uavcan.protocol.NodeStatus
    }
    if (!f.extended)
    {
        if (f.id == 0 && f.length == 2 && (f.data[0] == 1 || f.data[0] == 2 || f.data[0] == 0x80 || f.data[0] == 0x81 || f.data[0] == 0x82)) gOpen += 6;
        if (f.id >= 0x700 && f.id <= 0x77f && f.length == 1) gOpen += 3;
        if (((f.id >= 0x580 && f.id <= 0x67f) && f.length == 8 && IsCanOpenSdo(f.data[0]))) gOpen += 5;
    }
    if (f.extended)
    {
        uint32_t pgn = (f.id >> 8) & 0x3ffff;
        if (pgn == 0x0f004 || pgn == 0x0feca || pgn == 0x0fef2) gJ1939 += 8;
        else if (((f.id >> 16) & 0xff) >= 0xf0) gJ1939++;
    }
    gLastId = f.id; gLastExtended = f.extended; gLastLength = f.length;
    for (uint8_t i = 0; i < f.length; i++) gLastData[i] = f.data[i];
}

EProtocol Detect()
{
    int best = 0; EProtocol protocol = ProtocolNone;
    if (gHitec > best) { best = gHitec; protocol = ProtocolHitec; }
    if (gDrone > best) { best = gDrone; protocol = ProtocolDroneCan; }
    if (gOpen > best) { best = gOpen; protocol = ProtocolCanOpen; }
    if (gJ1939 > best) { best = gJ1939; protocol = ProtocolJ1939; }
    if (!gFrames) return ProtocolNone;
    return best >= 4 ? protocol : ProtocolUnknown;
}

const char* ProtocolName(EProtocol protocol)
{
    switch (protocol)
    {
        case ProtocolHitec: return "Hitec CAN Custom";
        case ProtocolDroneCan: return "DroneCAN / UAVCAN v0";
        case ProtocolCanOpen: return "CANopen";
        case ProtocolJ1939: return "SAE J1939";
        case ProtocolUnknown: return "unknown CAN traffic";
        default: return "waiting for frames";
    }
}

const char* ProtocolStatusName(EProtocol protocol)
{
    switch (protocol)
    {
        case ProtocolHitec: return "HITEC";
        case ProtocolDroneCan: return "DRONECAN";
        case ProtocolCanOpen: return "CANOPEN";
        case ProtocolJ1939: return "J1939";
        case ProtocolUnknown: return "UNKNOWN";
        default: return "WAIT";
    }
}

void Clear()
{
    gFrames = gHitec = gDrone = gOpen = gJ1939 = gExtended = gStandard = 0;
    gLastLength = 0;
}

void Configure()
{
    gCan.End(); gOnline = gCan.Begin(kBauds[gBaudIndex]); gBaudStart = SYS::GetTick();
}

void StartAuto()
{
    gAuto = true; gBaudIndex = 0; Clear(); Configure();
}

void NextBaud()
{
    gAuto = false; gBaudIndex = (gBaudIndex + 1) % kBaudCount; Clear(); Configure();
}

void DrawFrame()
{
    if (!gLastLength) return;
    int x = LCD::Printf(8, 184, RGB565(b0b0b0), RGBTRANS, "Last %s %s ", gLastExtended ? "EXT" : "STD", gLastExtended ? "%08X" : "%03X", gLastId);
    for (uint8_t i = 0; i < gLastLength; i++) x += LCD::Printf(x, 184, RGB565(ffffff), RGBTRANS, "%02X ", gLastData[i]);
}

void Draw()
{
    LCD::Bar(CRect(0, 0, LCD::Width, LCD::Height), RGB565(202020));
    LCD::Bar(CRect(0, 0, LCD::Width, 14), RGB565(4040a0));
    LCD::Print(8, 0, RGB565(ffffff), RGBTRANS, "Servo Protocol Probe (passive)");
    LCD::Print(8, 16, RGB565(ffd060), RGBTRANS, "P3=CAN_RX  P4=CAN_TX  no TX and no ACK");
    LCD::Printf(8, 32, gOnline ? RGB565(ffffff) : RGB565(ff6060), RGBTRANS, "Bus: %s  %s  Frames: %d", BaudName(kBauds[gBaudIndex]), gAuto ? "AUTO" : "LOCK", (int)gFrames);
    EProtocol detected = Detect();
    LCD::Printf(8, 54, detected == ProtocolNone ? RGB565(b0b0b0) : RGB565(00e000), RGBTRANS, "Result: %s", ProtocolName(detected));
    LCD::Printf(8, 76, RGB565(b0b0b0), RGBTRANS, "Hitec custom: %d    DroneCAN: %d", gHitec, gDrone);
    LCD::Printf(8, 94, RGB565(b0b0b0), RGBTRANS, "CANopen: %d         J1939: %d", gOpen, gJ1939);
    LCD::Printf(8, 112, RGB565(b0b0b0), RGBTRANS, "STD: %d  EXT: %d  ESR: %02X", gStandard, gExtended, (unsigned)(gCan.ErrorStatus() & 0xff));
    if (detected == ProtocolHitec)
        LCD::Print(8, 136, RGB565(ffffff), RGBTRANS, "Use MDB950 Doctor for safe read-only diagnostics.");
    else if (detected == ProtocolDroneCan)
        LCD::Print(8, 136, RGB565(ffffff), RGBTRANS, "DroneCAN detected: use a DroneCAN controller, not Hitec Custom.");
    else
        LCD::Print(8, 136, RGB565(808080), RGBTRANS, "Passive probe needs a device that emits CAN traffic.");
    LCD::Print(8, 154, RGB565(808080), RGBTRANS, "A silent servo cannot be identified without an active request.");
    DrawFrame();
    uint16_t bar = !gOnline ? RGB565(802020) : detected == ProtocolNone ? RGB565(505050) : RGB565(206020);
    LCD::Bar(CRect(0, 202, LCD::Width, 222), bar);
    LCD::Printf(8, 205, RGB565(ffffff), RGBTRANS, "STATUS  %s | %s | %s | ESR %02X",
        gOnline ? "CAN OK" : "CAN ERROR", BaudName(kBauds[gBaudIndex]), ProtocolStatusName(detected),
        (unsigned)(gCan.ErrorStatus() & 0xff));
    LCD::Print(8, 226, RGB565(b0b0b0), RGBTRANS, "F1: baud  F3: clear  F4: auto  F2/Esc: exit");
}

void Tick()
{
    TCanFrame frame;
    while (gOnline && gCan.Receive(frame)) Classify(frame);
    if (gAuto && !gFrames && SYS::GetTick() - gBaudStart >= 1500)
    {
        gBaudIndex = (gBaudIndex + 1) % kBaudCount;
        Configure();
    }
    else if (gAuto && gFrames) gAuto = false;
}
}

#ifdef _ARM
__attribute__((__section__(".entry")))
#endif
int _main(void)
{
    StartAuto(); uint32_t redraw = 0; BIOS::KEY::EKey key;
    while (true)
    {
        key = KEY::GetKey();
        if (key == KEY::Escape || key == KEY::F2) break;
        if (key == KEY::F1) NextBaud();
        else if (key == KEY::F3) Clear();
        else if (key == KEY::F4) StartAuto();
        Tick();
        if (key != KEY::None || SYS::GetTick() - redraw >= 120) { redraw = SYS::GetTick(); Draw(); }
    }
    gCan.End(); return 0;
}

void _HandleAssertion(const char* file, int line, const char* cond)
{
    BIOS::DBG::Print("Assertion failed in %s [%d]: %s\n", file, line, cond); while (1) {}
}
