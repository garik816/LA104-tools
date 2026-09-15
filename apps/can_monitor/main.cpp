#include <library.h>
#include "can.h"

using namespace BIOS;

namespace
{
const uint32_t kBauds[] = {125000, 250000, 500000, 1000000};
const int kBaudCount = sizeof(kBauds) / sizeof(kBauds[0]);
const int kMaxIds = 12;
const int kHistorySize = 5;

enum EProtocol
{
    ProtocolRaw,
    ProtocolCanOpen,
    ProtocolExtended,
    ProtocolJ1939
};

struct TIdInfo
{
    uint32_t id;
    uint32_t count;
    bool extended;
};

CCan gCan;
TIdInfo gIds[kMaxIds];
TCanFrame gHistory[kHistorySize];
int gHistoryCount = 0;
uint32_t gFrames = 0;
uint32_t gLastFrameTick = 0;
uint32_t gBaudStartTick = 0;
int gBaudIndex = 0;
bool gAutoBaud = true;
bool gPaused = false;
bool gOnline = false;
EProtocol gProtocol = ProtocolRaw;

const char* BaudName(uint32_t baud)
{
    switch (baud)
    {
        case 125000: return "125k";
        case 250000: return "250k";
        case 500000: return "500k";
        case 1000000: return "1M";
        default: return "?";
    }
}

const char* ProtocolName()
{
    switch (gProtocol)
    {
        case ProtocolCanOpen: return "CANopen candidate";
        case ProtocolJ1939: return "J1939 candidate";
        case ProtocolExtended: return "extended CAN traffic";
        default: return "raw CAN";
    }
}

void ClearCapture()
{
    memset(gIds, 0, sizeof(gIds));
    memset(gHistory, 0, sizeof(gHistory));
    gHistoryCount = 0;
    gFrames = 0;
    gLastFrameTick = 0;
    gProtocol = ProtocolRaw;
}

bool ConfigureBus()
{
    gCan.End();
    gOnline = gCan.Begin(kBauds[gBaudIndex]);
    gBaudStartTick = SYS::GetTick();
    return gOnline;
}

void StartAutoBaud()
{
    gAutoBaud = true;
    gPaused = false;
    gBaudIndex = 0;
    ClearCapture();
    ConfigureBus();
}

void NextManualBaud()
{
    gAutoBaud = false;
    gPaused = false;
    gBaudIndex = (gBaudIndex + 1) % kBaudCount;
    ClearCapture();
    ConfigureBus();
}

void LearnProtocol(const TCanFrame& frame)
{
    if (frame.extended)
    {
        if (frame.id >= 0x18f00000 && frame.id <= 0x1cffffff)
            gProtocol = ProtocolJ1939;
        else if (gProtocol == ProtocolRaw)
            gProtocol = ProtocolExtended;
        return;
    }

    // CANopen predefined connection set: NMT, PDO, SDO, heartbeat.
    if (frame.id == 0x000 ||
        (frame.id >= 0x080 && frame.id <= 0x5ff) ||
        (frame.id >= 0x700 && frame.id <= 0x77f))
        gProtocol = ProtocolCanOpen;
}

void AddId(const TCanFrame& frame)
{
    for (int i = 0; i < kMaxIds; i++)
    {
        if (gIds[i].count && gIds[i].id == frame.id && gIds[i].extended == frame.extended)
        {
            gIds[i].count++;
            return;
        }
    }

    for (int i = 0; i < kMaxIds; i++)
    {
        if (!gIds[i].count)
        {
            gIds[i].id = frame.id;
            gIds[i].count = 1;
            gIds[i].extended = frame.extended;
            return;
        }
    }
}

void AddFrame(const TCanFrame& frame)
{
    for (int i = kHistorySize - 1; i > 0; i--)
        gHistory[i] = gHistory[i - 1];
    gHistory[0] = frame;
    if (gHistoryCount < kHistorySize)
        gHistoryCount++;
    gFrames++;
    gLastFrameTick = SYS::GetTick();
    LearnProtocol(frame);
    AddId(frame);
}

void DrawHeader()
{
    LCD::Bar(CRect(0, 0, LCD::Width, 14), RGB565(4040a0));
    LCD::Print(8, 0, RGB565(ffffff), RGBTRANS, "CAN Monitor");
}

void DrawFrame(int y, const TCanFrame& frame)
{
    int x = 8;
    if (frame.extended)
        x += LCD::Printf(x, y, RGB565(ffffff), RGBTRANS, "%08X", frame.id);
    else
        x += LCD::Printf(x, y, RGB565(ffffff), RGBTRANS, "%03X", frame.id);
    x += LCD::Printf(x, y, RGB565(808080), RGBTRANS, " %c%u ", frame.remote ? 'R' : 'D', frame.length);
    for (int i = 0; i < frame.length; i++)
        x += LCD::Printf(x, y, RGB565(b0b0b0), RGBTRANS, "%02X ", frame.data[i]);
}

void DrawScreen()
{
    LCD::Bar(CRect(0, 0, LCD::Width, LCD::Height), RGB565(202020));
    DrawHeader();
    LCD::Print(8, 16, RGB565(b0b0b0), RGBTRANS, "P3=CAN_RX  P4=CAN_TX  passive only");
    LCD::Printf(8, 30, gOnline ? RGB565(ffffff) : RGB565(ff8080), RGBTRANS,
        "Bus: %s  %s", BaudName(kBauds[gBaudIndex]), gAutoBaud ? "AUTO" : "LOCKED");
    LCD::Printf(8, 44, RGB565(ffffff), RGBTRANS, "Frames: %d   State: %s", (int)gFrames,
        gPaused ? "PAUSED" : (gOnline ? "LISTEN" : "ERROR"));
    LCD::Printf(8, 58, RGB565(00e000), RGBTRANS, "Protocol: %s", ProtocolName());

    LCD::Print(8, 74, RGB565(808080), RGBTRANS, "Seen IDs:");
    int x = 80;
    int shown = 0;
    for (int i = 0; i < kMaxIds && shown < 5; i++)
    {
        if (!gIds[i].count)
            continue;
        x += LCD::Printf(x, 74, RGB565(b0b0b0), RGBTRANS, gIds[i].extended ? "%08X " : "%03X ", gIds[i].id);
        shown++;
    }

    LCD::Print(8, 96, RGB565(808080), RGBTRANS, "Latest frames:");
    for (int i = 0; i < gHistoryCount; i++)
        DrawFrame(110 + i * 18, gHistory[i]);

    uint32_t errors = gCan.ErrorStatus();
    LCD::Printf(8, 204, errors ? RGB565(ff8080) : RGB565(808080), RGBTRANS,
        "CAN ESR: 0x%02X", (unsigned)(errors & 0xff));
    LCD::Print(8, 226, RGB565(b0b0b0), RGBTRANS, "F1: baud  F3: pause  F4: auto  F2: exit");
}

void Tick()
{
    if (!gPaused && gOnline)
    {
        TCanFrame frame;
        while (gCan.Receive(frame))
            AddFrame(frame);
    }

    // Listen long enough for common periodic status frames, then try next rate.
    if (gAutoBaud && !gFrames && SYS::GetTick() - gBaudStartTick >= 1500)
    {
        gBaudIndex = (gBaudIndex + 1) % kBaudCount;
        ConfigureBus();
    }
    else if (gAutoBaud && gFrames)
        gAutoBaud = false;
}
}

#ifdef _ARM
__attribute__((__section__(".entry")))
#endif
int _main(void)
{
    StartAutoBaud();
    uint32_t redrawTick = 0;
    BIOS::KEY::EKey key;
    while ((key = KEY::GetKey()) != KEY::Escape)
    {
        if (key == KEY::F1)
            NextManualBaud();
        else if (key == KEY::F3)
            gPaused = !gPaused;
        else if (key == KEY::F4)
            StartAutoBaud();

        Tick();
        if (key != KEY::None || SYS::GetTick() - redrawTick >= 150)
        {
            redrawTick = SYS::GetTick();
            DrawScreen();
        }
    }

    gCan.End();
    return 0;
}

void _HandleAssertion(const char* file, int line, const char* cond)
{
    BIOS::DBG::Print("Assertion failed in %s [%d]: %s\n", file, line, cond);
    while (1)
    {
    }
}

