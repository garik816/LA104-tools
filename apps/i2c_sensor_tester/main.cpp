#include <library.h>

using namespace BIOS;

namespace
{
enum EDeviceKind
{
    DeviceUnknown,
    DeviceBmp280,
    DeviceBme280,
    DeviceDs3231,
    DeviceIna219,
    DeviceSsd1306
};

struct TDeviceInfo
{
    EDeviceKind kind;
    bool present;
    bool dataOk;
    uint8_t address;
    uint8_t chipId;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t status;
    uint16_t config;
    int32_t temperatureCenti;
    int32_t pressureRaw;
    uint16_t humidityRaw;
    int32_t busMilliVolts;
    int32_t shuntMicroVolts;
    int32_t rtcTemperatureQuarter;
};

const int kFirstAddress = 0x08;
const int kLastAddress = 0x77;
const int kMaxDevices = kLastAddress - kFirstAddress + 1;

uint8_t gDevices[kMaxDevices];
int gDeviceCount = 0;
int gSelected = 0;
TDeviceInfo gInfo;

bool I2cPresent(uint8_t address)
{
    if (!GPIO::I2C::BeginTransmission(address))
        return false;
    return GPIO::I2C::EndTransmission(true);
}

bool I2cRead(uint8_t address, uint8_t reg, uint8_t* data, uint8_t length)
{
    if (!GPIO::I2C::BeginTransmission(address))
        return false;

    if (!GPIO::I2C::Write(reg))
    {
        GPIO::I2C::EndTransmission(true);
        return false;
    }

    if (!GPIO::I2C::EndTransmission(false))
    {
        GPIO::I2C::EndTransmission(true);
        return false;
    }

    if (!GPIO::I2C::RequestFrom(address, length))
    {
        GPIO::I2C::EndTransmission(true);
        return false;
    }

    for (uint8_t i = 0; i < length; i++)
        data[i] = GPIO::I2C::Read();

    return GPIO::I2C::EndTransmission(true);
}

uint16_t ReadBe16(const uint8_t* data)
{
    return (uint16_t)((data[0] << 8) | data[1]);
}

uint16_t ReadLe16(const uint8_t* data)
{
    return (uint16_t)(data[0] | (data[1] << 8));
}

int16_t ReadLeS16(const uint8_t* data)
{
    return (int16_t)ReadLe16(data);
}

int BcdToInt(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0f);
}

bool IsBcdInRange(uint8_t value, int minimum, int maximum)
{
    if ((value & 0x0f) > 9)
        return false;
    int decimal = BcdToInt(value);
    return decimal >= minimum && decimal <= maximum;
}

bool IsDs3231Time(const uint8_t* data)
{
    uint8_t hours = data[2] & 0x3f;
    if (data[2] & 0x40)
        hours &= 0x1f;

    return IsBcdInRange(data[0] & 0x7f, 0, 59) &&
           IsBcdInRange(data[1] & 0x7f, 0, 59) &&
           IsBcdInRange(hours, 1, (data[2] & 0x40) ? 12 : 23) &&
           ((data[3] & 0x07) >= 1 && (data[3] & 0x07) <= 7) &&
           IsBcdInRange(data[4] & 0x3f, 1, 31) &&
           IsBcdInRange(data[5] & 0x1f, 1, 12);
}

void ResetInfo(uint8_t address)
{
    memset(&gInfo, 0, sizeof(gInfo));
    gInfo.kind = DeviceUnknown;
    gInfo.address = address;
    gInfo.present = true;
}

void ProbeBmp(uint8_t address)
{
    uint8_t id;
    if (!I2cRead(address, 0xd0, &id, 1))
        return;

    if (id != 0x56 && id != 0x57 && id != 0x58 && id != 0x60)
        return;

    uint8_t calibration[6];
    uint8_t raw[8];
    if (!I2cRead(address, 0x88, calibration, sizeof(calibration)) ||
        !I2cRead(address, 0xf7, raw, sizeof(raw)))
        return;

    int32_t adcTemperature = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t t1 = ReadLe16(calibration);
    int32_t t2 = ReadLeS16(calibration + 2);
    int32_t t3 = ReadLeS16(calibration + 4);
    int32_t value = adcTemperature >> 3;
    int32_t var1 = ((value - (t1 << 1)) * t2) >> 11;
    value = adcTemperature >> 4;
    int32_t var2 = (((value - t1) * (value - t1)) >> 12) * t3 >> 14;

    gInfo.kind = id == 0x60 ? DeviceBme280 : DeviceBmp280;
    gInfo.chipId = id;
    gInfo.temperatureCenti = ((var1 + var2) * 5 + 128) >> 8;
    gInfo.pressureRaw = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    gInfo.humidityRaw = (uint16_t)((raw[6] << 8) | raw[7]);
    gInfo.dataOk = true;
}

void ProbeDs3231(uint8_t address)
{
    if (address != 0x68)
        return;

    uint8_t time[7];
    uint8_t temperature[2];
    uint8_t status;
    if (!I2cRead(address, 0x00, time, sizeof(time)) ||
        !I2cRead(address, 0x0f, &status, 1) ||
        !I2cRead(address, 0x11, temperature, sizeof(temperature)))
        return;

    // DS3231 bits 6:4 of the status register are reserved and read zero.
    if (!IsDs3231Time(time) || (status & 0x70) != 0)
        return;

    uint8_t hour = time[2] & 0x3f;
    int hourDecimal;
    if (time[2] & 0x40)
    {
        hour &= 0x1f;
        hourDecimal = BcdToInt(hour);
        if (hourDecimal == 12)
            hourDecimal = 0;
        if (time[2] & 0x20)
            hourDecimal += 12;
    }
    else
        hourDecimal = BcdToInt(hour);

    gInfo.kind = DeviceDs3231;
    gInfo.seconds = (uint8_t)BcdToInt(time[0] & 0x7f);
    gInfo.minutes = (uint8_t)BcdToInt(time[1] & 0x7f);
    gInfo.hours = (uint8_t)hourDecimal;
    gInfo.day = (uint8_t)BcdToInt(time[4] & 0x3f);
    gInfo.month = (uint8_t)BcdToInt(time[5] & 0x1f);
    gInfo.year = (uint8_t)BcdToInt(time[6]);
    gInfo.status = status;
    gInfo.rtcTemperatureQuarter = ((int8_t)temperature[0] * 4) + ((temperature[1] >> 6) & 3);
    gInfo.dataOk = true;
}

void ProbeIna219(uint8_t address)
{
    if (address < 0x40 || address > 0x4f)
        return;

    uint8_t config[2];
    uint8_t bus[2];
    uint8_t shunt[2];
    if (!I2cRead(address, 0x00, config, sizeof(config)) ||
        !I2cRead(address, 0x01, shunt, sizeof(shunt)) ||
        !I2cRead(address, 0x02, bus, sizeof(bus)))
        return;

    uint16_t configValue = ReadBe16(config);
    if ((configValue & 0x0007) == 0)
        return;

    gInfo.kind = DeviceIna219;
    gInfo.config = configValue;
    gInfo.busMilliVolts = (int32_t)(ReadBe16(bus) >> 3) * 4;
    gInfo.shuntMicroVolts = (int32_t)(int16_t)ReadBe16(shunt) * 10;
    gInfo.dataOk = true;
}

void ProbeDevice(uint8_t address)
{
    ResetInfo(address);

    if (address == 0x76 || address == 0x77)
        ProbeBmp(address);
    if (gInfo.kind == DeviceUnknown)
        ProbeDs3231(address);
    if (gInfo.kind == DeviceUnknown)
        ProbeIna219(address);
    if (gInfo.kind == DeviceUnknown && (address == 0x3c || address == 0x3d))
        gInfo.kind = DeviceSsd1306;
}

const char* DeviceName()
{
    switch (gInfo.kind)
    {
        case DeviceBmp280: return "BMP280";
        case DeviceBme280: return "BME280";
        case DeviceDs3231: return "DS3231 compatible";
        case DeviceIna219: return "INA219 compatible";
        case DeviceSsd1306: return "SSD1306/SH1106 candidate";
        default: return "Unknown device";
    }
}

void PrintTemperature(int x, int y, const char* label, int32_t centi)
{
    int32_t absolute = centi < 0 ? -centi : centi;
    LCD::Printf(x, y, RGB565(ffffff), RGBTRANS, "%s%s%d.%02d C", label,
        centi < 0 ? "-" : "", (int)(absolute / 100), (int)(absolute % 100));
}

void DrawHeader(const char* title)
{
    LCD::Bar(CRect(0, 0, LCD::Width, 14), RGB565(4040a0));
    LCD::Print(8, 0, RGB565(ffffff), RGBTRANS, title);
}

void DrawScanning(int address)
{
    LCD::Bar(CRect(0, 14, LCD::Width, LCD::Height), RGB565(202020));
    LCD::Printf(8, 30, RGB565(ffffff), RGBTRANS, "Scanning 0x%02X...", address);
}

void Scan()
{
    gDeviceCount = 0;
    gSelected = 0;
    DrawHeader("I2C Sensor Tester");

    for (int address = kFirstAddress; address <= kLastAddress; address++)
    {
        if ((address & 0x0f) == 0)
            DrawScanning(address);
        if (I2cPresent((uint8_t)address) && gDeviceCount < kMaxDevices)
            gDevices[gDeviceCount++] = (uint8_t)address;
    }

    if (gDeviceCount)
        ProbeDevice(gDevices[gSelected]);
}

void DrawDeviceList()
{
    LCD::Bar(CRect(8, 42, LCD::Width - 8, 70), RGB565(202020));
    if (!gDeviceCount)
    {
        LCD::Print(8, 48, RGB565(ff8080), RGBTRANS, "No I2C address ACKed");
        return;
    }

    int start = (gSelected / 8) * 8;
    for (int i = start; i < gDeviceCount && i < start + 8; i++)
    {
        int x = 8 + (i - start) * 39;
        bool selected = i == gSelected;
        LCD::Printf(x, 48, selected ? RGB565(000000) : RGB565(d0d0d0),
            selected ? RGB565(ffffff) : RGB565(202020), "%02X", gDevices[i]);
    }
}

void DrawDetails()
{
    LCD::Bar(CRect(0, 70, LCD::Width, 224), RGB565(202020));
    if (!gDeviceCount)
        return;

    LCD::Printf(8, 76, RGB565(ffffff), RGBTRANS, "Address: 0x%02X", gInfo.address);
    LCD::Printf(8, 90, RGB565(ffffff), RGBTRANS, "Device:  %s", DeviceName());
    LCD::Print(8, 104, RGB565(00e000), RGBTRANS, "Probe:   ACK (address write)");

    switch (gInfo.kind)
    {
        case DeviceBmp280:
        case DeviceBme280:
            LCD::Printf(8, 120, RGB565(ffffff), RGBTRANS, "Chip ID: 0x%02X", gInfo.chipId);
            if (gInfo.dataOk)
            {
                PrintTemperature(8, 134, "Temp:    ", gInfo.temperatureCenti);
                if (gInfo.kind == DeviceBme280)
                    LCD::Printf(8, 148, RGB565(b0b0b0), RGBTRANS, "Raw P: %d  Raw H: %u",
                        (int)gInfo.pressureRaw, (unsigned)gInfo.humidityRaw);
                else
                    LCD::Printf(8, 148, RGB565(b0b0b0), RGBTRANS, "Raw pressure: %d", (int)gInfo.pressureRaw);
                LCD::Print(8, 162, RGB565(808080), RGBTRANS, "Read-only sample; sensor config unchanged");
            }
            break;

        case DeviceDs3231:
            LCD::Printf(8, 120, RGB565(ffffff), RGBTRANS, "Time:    20%02u-%02u-%02u %02u:%02u:%02u",
                (unsigned)gInfo.year, (unsigned)gInfo.month, (unsigned)gInfo.day,
                (unsigned)gInfo.hours, (unsigned)gInfo.minutes, (unsigned)gInfo.seconds);
            PrintTemperature(8, 134, "Temp:    ", gInfo.rtcTemperatureQuarter * 25);
            LCD::Printf(8, 148, RGB565(b0b0b0), RGBTRANS, "Status:  0x%02X  (no unique chip ID)", gInfo.status);
            break;

        case DeviceIna219:
            LCD::Printf(8, 120, RGB565(ffffff), RGBTRANS, "Config:  0x%04X", gInfo.config);
            LCD::Printf(8, 134, RGB565(ffffff), RGBTRANS, "Bus:     %d.%03d V", (int)(gInfo.busMilliVolts / 1000),
                (int)(gInfo.busMilliVolts % 1000));
            LCD::Printf(8, 148, RGB565(ffffff), RGBTRANS, "Shunt:   %d uV", (int)gInfo.shuntMicroVolts);
            LCD::Print(8, 162, RGB565(808080), RGBTRANS, "Candidate: INA219 has no ID register");
            break;

        case DeviceSsd1306:
            LCD::Print(8, 120, RGB565(ffffff), RGBTRANS, "Display controller address ACKed");
            LCD::Print(8, 134, RGB565(b0b0b0), RGBTRANS, "No commands or display data sent");
            LCD::Print(8, 148, RGB565(808080), RGBTRANS, "SSD1306 and SH1106 share this address");
            break;

        default:
            LCD::Print(8, 120, RGB565(ffffff), RGBTRANS, "No safe ID-register probe is known");
            LCD::Print(8, 134, RGB565(b0b0b0), RGBTRANS, "Result: address ACK only");
            break;
    }
}

void DrawScreen()
{
    LCD::Bar(CRect(0, 0, LCD::Width, LCD::Height), RGB565(202020));
    DrawHeader("I2C Sensor Tester");
    LCD::Print(8, 16, RGB565(b0b0b0), RGBTRANS, "P1=SCL  P2=SDA   F1:SCAN  F3:READ");
    LCD::Printf(8, 30, RGB565(ffffff), RGBTRANS, "Found: %d", gDeviceCount);
    if (gDeviceCount)
        LCD::Printf(104, 30, RGB565(b0b0b0), RGBTRANS, "Selected: %d/%d", gSelected + 1, gDeviceCount);
    DrawDeviceList();
    DrawDetails();
    LCD::Print(8, 226, RGB565(b0b0b0), RGBTRANS, "UP/DN: select    F2: exit");
}

void InitI2c()
{
    GPIO::PinMode(GPIO::P1, GPIO::I2c);
    GPIO::PinMode(GPIO::P2, GPIO::I2c);
}

void DeinitI2c()
{
    GPIO::PinMode(GPIO::P1, GPIO::Input);
    GPIO::PinMode(GPIO::P2, GPIO::Input);
}
}

#ifdef _ARM
__attribute__((__section__(".entry")))
#endif
int _main(void)
{
    InitI2c();
    Scan();
    DrawScreen();

    BIOS::KEY::EKey key;
    while ((key = KEY::GetKey()) != KEY::Escape)
    {
        if (key == KEY::F1)
        {
            Scan();
            DrawScreen();
        }
        else if (key == KEY::F3 && gDeviceCount)
        {
            ProbeDevice(gDevices[gSelected]);
            DrawScreen();
        }
        else if (key == KEY::Down && gDeviceCount && gSelected + 1 < gDeviceCount)
        {
            gSelected++;
            ProbeDevice(gDevices[gSelected]);
            DrawScreen();
        }
        else if (key == KEY::Up && gDeviceCount && gSelected > 0)
        {
            gSelected--;
            ProbeDevice(gDevices[gSelected]);
            DrawScreen();
        }
    }

    DeinitI2c();
    return 0;
}

void _HandleAssertion(const char* file, int line, const char* cond)
{
    BIOS::DBG::Print("Assertion failed in %s [%d]: %s\n", file, line, cond);
    while (1)
    {
    }
}

