#include "button.h"

// HalButton实现
HalButton::HalButton(const char *name, GPIO_TypeDef *port, uint16_t pin)
    : m_Name(name), m_Port(port), m_Pin(pin), m_LastState(false), m_PressedEdge(false)
{
}

void HalButton::Update()
{
    bool current = IsPressed();
    m_PressedEdge = (!m_LastState && current);
    m_LastState = current;
}

bool HalButton::IsPressed() const
{
    return HAL_GPIO_ReadPin(m_Port, m_Pin) == GPIO_PIN_RESET; // 假设低电平为按下
}

bool HalButton::WasPressed() const
{
    return m_PressedEdge;
}

const char *HalButton::GetName() const
{
    return m_Name;
}

// HalSensor实现
HalSensor::HalSensor(const char *name, GPIO_TypeDef *port, uint16_t pin) : m_Name(name), m_Port(port), m_Pin(pin)
{
}

bool HalSensor::IsTrigger() const
{
    return HAL_GPIO_ReadPin(m_Port, m_Pin) == GPIO_PIN_SET;
}

const char *HalSensor::GetName() const
{
    return m_Name;
}

// HalValve实现
HalValve::HalValve(const char *name, GPIO_TypeDef *port, uint16_t pin, bool active_high)
    : m_Name(name), m_ActiveHigh(active_high), m_Port(port), m_Pin(pin), m_IsOpenState(false)
{
}

void HalValve::Open()
{
    HAL_GPIO_WritePin(m_Port, m_Pin, m_ActiveHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
    m_IsOpenState = true;
}

void HalValve::Close()
{
    HAL_GPIO_WritePin(m_Port, m_Pin, m_ActiveHigh ? GPIO_PIN_RESET : GPIO_PIN_SET);
    m_IsOpenState = false;
}

void HalValve::Toggle()
{
    HAL_GPIO_TogglePin(m_Port, m_Pin);
    m_IsOpenState = !m_IsOpenState;
}

bool HalValve::IsOpen() const
{
    return m_IsOpenState;
}

const char *HalValve::GetName() const
{
    return m_Name;
}

// HalDipSwitch实现
HalDipSwitch::HalDipSwitch(const DipSwitchInfo &info)
    : m_Keys{HalButton("DIP0", info.Pins[0].Port, info.Pins[0].Pin),
             HalButton("DIP1", info.Pins[1].Port, info.Pins[1].Pin),
             HalButton("DIP2", info.Pins[2].Port, info.Pins[2].Pin),
             HalButton("DIP3", info.Pins[3].Port, info.Pins[3].Pin),
             HalButton("DIP4", info.Pins[4].Port, info.Pins[4].Pin),
             HalButton("DIP5", info.Pins[5].Port, info.Pins[5].Pin),
             HalButton("DIP6", info.Pins[6].Port, info.Pins[6].Pin),
             HalButton("DIP7", info.Pins[7].Port, info.Pins[7].Pin)}
{
}

bool HalDipSwitch::IsOn(int index) const
{
    if (index < 0 || index >= 8)
        return false;
    return m_Keys[index].IsPressed();
}

uint8_t HalDipSwitch::Value() const
{
    uint8_t val = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (IsOn(i))
        {
            val |= (1 << i);
        }
    }
    return val;
}
