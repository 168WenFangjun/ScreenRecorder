#pragma once

#include "KeyInterface.h"

struct _XDisplay;
class X11Key : public KeyInterface
{
public:
    X11Key(QObject *parent);
    virtual ~X11Key();

protected slots:
    virtual void testHotKeyPress() override;
    virtual void onRegisterHotKey(quint32 key, quint32 modifiers) override;
    virtual void onUnRegisterHotKey(quint32 key, quint32 modifiers) override;

protected:
    _XDisplay* m_dpy;
    int m_registered_hotkey_count;
};
