#include "can.h"

extern "C"
{
#include "stm32f10x_can.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
}

namespace { const uint32_t kCanClockHz = 36000000; const uint8_t kTimeQuanta = 18; }

bool CCan::Begin(uint32_t baud)
{
    if (!baud || kCanClockHz % (baud * kTimeQuanta)) return false;
    uint32_t prescaler = kCanClockHz / (baud * kTimeQuanta);
    if (!prescaler || prescaler > 1024) return false;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_8; gpio.GPIO_Speed = GPIO_Speed_50MHz; gpio.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(GPIOB, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_9; gpio.GPIO_Mode = GPIO_Mode_AF_PP; GPIO_Init(GPIOB, &gpio);
    CAN_DeInit(CAN1);
    CAN_InitTypeDef init; CAN_StructInit(&init);
    init.CAN_Mode = CAN_Mode_Silent;
    init.CAN_SJW = CAN_SJW_1tq; init.CAN_BS1 = CAN_BS1_13tq; init.CAN_BS2 = CAN_BS2_4tq;
    init.CAN_Prescaler = (uint16_t)prescaler; init.CAN_ABOM = ENABLE; init.CAN_NART = ENABLE;
    if (CAN_Init(CAN1, &init) != CAN_InitStatus_Success) return false;
    CAN_FilterInitTypeDef filter;
    filter.CAN_FilterNumber = 0; filter.CAN_FilterMode = CAN_FilterMode_IdMask; filter.CAN_FilterScale = CAN_FilterScale_32bit;
    filter.CAN_FilterIdHigh = 0; filter.CAN_FilterIdLow = 0; filter.CAN_FilterMaskIdHigh = 0; filter.CAN_FilterMaskIdLow = 0;
    filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0; filter.CAN_FilterActivation = ENABLE; CAN_FilterInit(&filter);
    return true;
}

void CCan::End()
{
    CAN_DeInit(CAN1); GPIO_PinRemapConfig(GPIO_Remap1_CAN1, DISABLE);
    GPIO_InitTypeDef gpio; gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOB, &gpio);
}

bool CCan::Receive(TCanFrame& frame)
{
    if (!CAN_MessagePending(CAN1, CAN_FIFO0)) return false;
    CanRxMsg rx; CAN_Receive(CAN1, CAN_FIFO0, &rx);
    frame.extended = rx.IDE == CAN_ID_EXT; frame.remote = rx.RTR == CAN_RTR_REMOTE;
    frame.id = frame.extended ? rx.ExtId : rx.StdId; frame.length = rx.DLC > 8 ? 8 : rx.DLC;
    for (uint8_t i = 0; i < frame.length; i++) frame.data[i] = rx.Data[i];
    return true;
}

uint32_t CCan::ErrorStatus() const { return CAN1->ESR; }
