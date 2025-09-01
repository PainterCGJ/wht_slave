#include "LockController.h"

LockController::LockController(const char *name, HalButton &keyButton, HalButton &unlockButton, HalValve &lockValve)
    : m_Name(name), m_KeyButton(keyButton), m_UnlockButton(unlockButton), m_LockValve(lockValve),
      m_State(LockState::Unlocked)
{
}

void LockController::Update()
{
    // 更新按钮边沿状态
    m_KeyButton.Update();
    m_UnlockButton.Update();

    // 钥匙按钮按下 → 上锁（打开阀门）
    if (m_KeyButton.WasPressed())
    {
        m_LockValve.Open();
        m_State = LockState::Locked;
    }

    // 解锁按钮按下 → 解锁（关闭阀门）
    if (m_UnlockButton.WasPressed())
    {
        m_LockValve.Close();
        m_State = LockState::Unlocked;
    }
}

void LockController::Reset()
{
    // 复位操作：解锁并关闭阀门
    m_LockValve.Close();
    m_State = LockState::Unlocked;
}

LockState LockController::GetState() const
{
    return m_State;
}

const char *LockController::GetName() const
{
    return m_Name;
}