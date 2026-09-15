#include <library.h>
#include "can.h"

using namespace BIOS;

namespace
{
const uint32_t kBauds[] = {250000, 500000, 125000, 1000000};
const int kBaudCount = sizeof(kBauds) / sizeof(kBauds[0]);
const uint32_t kScanIntervalMs = 18;
const uint32_t kPollIntervalMs = 150;
const uint32_t kArmDurationMs = 5000;
const int kPositionStep = 91; // About two degrees: 4096 counts = 90 degrees.

const uint8_t kRegPosition = 0x0c;
const uint8_t kRegVoltage = 0x12;
const uint8_t kRegMcuTemp = 0x14;
const uint8_t kRegCurrent = 0x16;
const uint8_t kRegVersion = 0xfc;
const uint8_t kRegVersionInverse = 0xfe;
const uint8_t kRegPositionNew = 0x1e;

CCan gCan;
bool gOnline = false;
bool gScanning = false;
int gBaudIndex = 0;
uint8_t gNextServoId = 1;
uint32_t gLastScanTick = 0;
uint32_t gLastPollTick = 0;
uint32_t gArmedUntil = 0;
uint8_t gServoId = 0;
bool gFound = false;
bool gHavePosition = false;
uint16_t gPosition = 0;
uint16_t gVoltage = 0;
int16_t gMcuTemp = 0;
uint16_t gCurrent = 0;
uint16_t gVersion = 0;
uint16_t gVersionInverse = 0;
uint32_t gFrames = 0;
const char* gStatus = "F1 starts safe scan";

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

bool IsArmed()
{
    return gArmedUntil && (int32_t)(gArmedUntil - SYS::GetTick()) > 0;
}

void ConfigureBus()
{
    gCan.End();
    gOnline = gCan.Begin(kBauds[gBaudIndex]);
}

void SendRead(uint8_t targetId, uint8_t address)
{
    TCanFrame frame;
    frame.id = 0; // Target ID2. Default/unconfigured Hitec CAN bus ID is 0.
    frame.extended = 0;
    frame.remote = 0;
    frame.length = 3;
    frame.data[0] = 'r';
    frame.data[1] = targetId;
    frame.data[2] = address;
    gCan.Transmit(frame);
}

void SendTwoReads(uint8_t addressA, uint8_t addressB)
{
    TCanFrame frame;
    frame.id = 0;
    frame.extended = 0;
    frame.remote = 0;
    frame.length = 4;
    frame.data[0] = 'R';
    frame.data[1] = gServoId;
    frame.data[2] = addressA;
    frame.data[3] = addressB;
    gCan.Transmit(frame);
}

void SendPosition(int delta)
{
    if (!gFound || !gHavePosition || !IsArmed())
        return;

    int target = (int)gPosition + delta;
    if (target < 0)
        target = 0;
    if (target > 16383)
        target = 16383;

    TCanFrame frame;
    frame.id = 0;
    frame.extended = 0;
    frame.remote = 0;
    frame.length = 5;
    frame.data[0] = 'w';
    frame.data[1] = gServoId;
    frame.data[2] = kRegPositionNew;
    frame.data[3] = (uint8_t)target;
    frame.data[4] = (uint8_t)(target >> 8);
    if (gCan.Transmit(frame))
        gStatus = delta < 0 ? "Sent -2 degree target" : "Sent +2 degree target";
    gArmedUntil = 0;
}

void StartScan()
{
    gScanning = true;
    gBaudIndex = 0;
    gNextServoId = 1;
    gFound = false;
    gHavePosition = false;
    gArmedUntil = 0;
    gStatus = "Scanning ID1=1..255";
    ConfigureBus();
}

void StopScan()
{
    gScanning = false;
    gStatus = "Scan stopped";
}

void SetValue(uint8_t address, uint16_t value)
{
    if (address == kRegPosition)
    {
        gPosition = value;
        gHavePosition = true;
    }
    else if (address == kRegVoltage)
        gVoltage = value;
    else if (address == kRegMcuTemp)
        gMcuTemp = (int16_t)value;
    else if (address == kRegCurrent)
        gCurrent = value;
    else if (address == kRegVersion)
        gVersion = value;
    else if (address == kRegVersionInverse)
        gVersionInverse = value;
}

void ProcessFrame(const TCanFrame& frame)
{
    gFrames++;
    if (frame.extended || frame.remote || frame.length < 5)
        return;

    if (frame.data[0] == 'v')
    {
        uint8_t id = frame.data[1];
        uint8_t address = frame.data[2];
        uint16_t value = (uint16_t)frame.data[3] | ((uint16_t)frame.data[4] << 8);
        if (address == kRegVersion && id != 0 && !gFound)
        {
            gFound = true;
            gScanning = false;
            gServoId = id;
            gStatus = "MDB/Hitec response found";
        }
        if (gFound && id == gServoId)
            SetValue(address, value);
    }
    else if (frame.data[0] == 'V' && frame.length >= 8 && gFound && frame.data[1] == gServoId)
    {
        SetValue(frame.data[2], (uint16_t)frame.data[3] | ((uint16_t)frame.data[4] << 8));
        SetValue(frame.data[5], (uint16_t)frame.data[6] | ((uint16_t)frame.data[7] << 8));
    }
}

void Tick()
{
    TCanFrame frame;
    while (gOnline && gCan.Receive(frame))
        ProcessFrame(frame);

    uint32_t now = SYS::GetTick();
    if (gScanning && now - gLastScanTick >= kScanIntervalMs)
    {
        gLastScanTick = now;
        SendRead(gNextServoId, kRegVersion);
        gNextServoId++;
        if (gNextServoId == 0)
        {
            gNextServoId = 1;
            gBaudIndex++;
            if (gBaudIndex >= kBaudCount)
            {
                gBaudIndex = 0;
                gStatus = "No ID1 response; set ID1 first";
            }
            ConfigureBus();
        }
    }

    if (gFound && now - gLastPollTick >= kPollIntervalMs)
    {
        gLastPollTick = now;
        static uint8_t phase = 0;
        if (phase == 0)
            SendTwoReads(kRegPosition, kRegVoltage);
        else if (phase == 1)
            SendTwoReads(kRegMcuTemp, kRegCurrent);
        else
            SendTwoReads(kRegVersion, kRegVersionInverse);
        phase = (phase + 1) % 3;
    }
}

void DrawScreen()
{
    LCD::Bar(CRect(0, 0, LCD::Width, LCD::Height), RGB565(202020));
    LCD::Bar(CRect(0, 0, LCD::Width, 14), RGB565(4040a0));
    LCD::Print(8, 0, RGB565(ffffff), RGBTRANS, "Hitec MDB950SW-CAN Doctor");
    LCD::Print(8, 16, RGB565(ffd060), RGBTRANS, "P3=CAN_RX P4=CAN_TX: external 3.3V transceiver only");
    LCD::Printf(8, 32, gOnline ? RGB565(ffffff) : RGB565(ff8080), RGBTRANS,
        "Bus ID2: 0  Baud: %s  %s", BaudName(kBauds[gBaudIndex]), gScanning ? "SCAN" : "READY");
    LCD::Printf(8, 48, RGB565(b0b0b0), RGBTRANS, "RX frames: %d  ESR: %02X", (int)gFrames,
        (unsigned)(gCan.ErrorStatus() & 0xff));

    if (!gFound)
    {
        LCD::Print(8, 76, RGB565(ffffff), RGBTRANS, "Read-only identification only");
        LCD::Printf(8, 94, RGB565(b0b0b0), RGBTRANS, "ID1 probe: %u / 255", (unsigned)gNextServoId);
        LCD::Print(8, 112, RGB565(808080), RGBTRANS, "ID1=0 cannot be safely enumerated on a shared bus.");
        LCD::Print(8, 128, RGB565(808080), RGBTRANS, "Configure it once with Hitec DPC-CAN, then scan.");
    }
    else
    {
        LCD::Printf(8, 70, RGB565(00e000), RGBTRANS, "FOUND: Hitec ID1=%u", (unsigned)gServoId);
        LCD::Printf(8, 88, RGB565(ffffff), RGBTRANS, "FW raw: %u  inverse: %u", (unsigned)gVersion, (unsigned)gVersionInverse);
        LCD::Printf(8, 106, gHavePosition ? RGB565(ffffff) : RGB565(808080), RGBTRANS,
            "Position: %u / 16383", (unsigned)gPosition);
        LCD::Printf(8, 124, RGB565(ffffff), RGBTRANS, "Voltage: %u.%02u V   Current: %u mA",
            (unsigned)(gVoltage / 100), (unsigned)(gVoltage % 100), (unsigned)gCurrent);
        LCD::Printf(8, 142, RGB565(ffffff), RGBTRANS, "MCU temp: %d C", (int)gMcuTemp);
        LCD::Printf(8, 164, IsArmed() ? RGB565(ff6060) : RGB565(808080), RGBTRANS,
            IsArmed() ? "ARMED: F3 -2deg, F4 +2deg (one command)" : "F2 arms a single +/-2 degree motion test");
    }

    LCD::Printf(8, 194, RGB565(b0b0b0), RGBTRANS, "%s", gStatus);
    LCD::Print(8, 212, RGB565(808080), RGBTRANS, "F1: scan/stop  F2: arm  F3: -2deg  F4: +2deg");
    LCD::Print(8, 226, RGB565(808080), RGBTRANS, "Esc: exit   No save/reset/factory commands are implemented.");
}
}

#ifdef _ARM
__attribute__((__section__(".entry")))
#endif
int _main(void)
{
    ConfigureBus();
    uint32_t redrawTick = 0;
    BIOS::KEY::EKey key;
    while ((key = KEY::GetKey()) != KEY::Escape)
    {
        if (key == KEY::F1)
        {
            if (gScanning)
                StopScan();
            else
                StartScan();
        }
        else if (key == KEY::F2 && gFound)
        {
            gArmedUntil = SYS::GetTick() + kArmDurationMs;
            gStatus = "ARM: clear mechanics, then choose direction";
        }
        else if (key == KEY::F3)
            SendPosition(-kPositionStep);
        else if (key == KEY::F4)
            SendPosition(kPositionStep);

        Tick();
        if (key != KEY::None || SYS::GetTick() - redrawTick >= 100)
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
    while (1) {}
}
