#pragma once

#include "KeyInterface.h"

class WindowsKey : public KeyInterface
{
public:
    WindowsKey(QObject *parent);

protected slots:
    virtual void testHotKeyPress() override;
    virtual void onRegisterHotKey(quint32 key, quint32 modifiers) override;
    virtual void onUnRegisterHotKey(quint32 key, quint32 modifiers) override;

protected:
    QList<QPair<quint32/*key*/, quint32/*modifiers*/>> m_hotkeys;
};
