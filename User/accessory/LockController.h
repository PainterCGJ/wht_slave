#pragma once

#include "button.h"

#ifndef LOCKCONTROLLER_HPP
#define LOCKCONTROLLER_HPP
enum class LockState
{
    Unlocked,
    Locked
};
#endif

class LockController
{
  public:
    LockController(const char *name, HalButton &keyButton, HalButton &unlockButton, HalValve &lockValve);

    void Update();
    void Reset(); // 复位函数：解锁并关闭阀门
    LockState GetState() const;
    const char *GetName() const;

  private:
    const char *m_Name;
    HalButton &m_KeyButton;    // 钥匙按钮传感器
    HalButton &m_UnlockButton; // 解锁按钮
    HalValve &m_LockValve;     // 锁控制阀门
    LockState m_State;
};