#pragma once

#include "stm32f4xx_hal.h"

// HAL层按钮实现
class HalButton
{
  public:
    HalButton(const char *name, GPIO_TypeDef *port, uint16_t pin);

    // 保持原有函数接口
    void Update();
    bool IsPressed() const;
    bool WasPressed() const;
    const char *GetName() const;

  private:
    const char *m_Name;
    GPIO_TypeDef *m_Port;
    uint16_t m_Pin;
    bool m_LastState;
    bool m_PressedEdge;
};

// HAL层传感器实现
class HalSensor
{
  public:
    HalSensor(const char *name, GPIO_TypeDef *port, uint16_t pin);

    // 保持原有函数接口
    bool IsTrigger() const;
    const char *GetName() const;

  private:
    const char *m_Name;
    GPIO_TypeDef *m_Port;
    uint16_t m_Pin;
};

// HAL层阀门实现
class HalValve
{
  public:
    HalValve(const char *name, GPIO_TypeDef *port, uint16_t pin, bool active_high = true);

    // 保持原有函数接口
    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const;
    const char *GetName() const;

  private:
    const char *m_Name;
    bool m_ActiveHigh;
    GPIO_TypeDef *m_Port;
    uint16_t m_Pin;
    bool m_IsOpenState;
};

// 拨码开关信息结构体
#ifndef DIPSWITCHINFO_DEFINED
#define DIPSWITCHINFO_DEFINED
struct DipSwitchInfo
{
    struct PinInfo
    {
        GPIO_TypeDef *Port;
        uint16_t Pin;
    };
    PinInfo Pins[8];
};
#endif

// HAL层拨码开关实现
class HalDipSwitch
{
  public:
    HalDipSwitch(const DipSwitchInfo &info);

    // 保持原有函数接口
    bool IsOn(int index) const;
    uint8_t Value() const;

  private:
    HalButton m_Keys[8];
};
