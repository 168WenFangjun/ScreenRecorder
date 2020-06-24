#include "WindowsKey.h"

#include <QDebug>
#include <QPixmap>
#include <windows.h>

WindowsKey::WindowsKey(QObject *parent)
    : KeyInterface(parent)
{
}

void WindowsKey::testHotKeyPress()
{
    MSG msg = {0};
    if(GetMessage(&msg, NULL, 0, 0) != 0)
    {
        if(msg.message == WM_HOTKEY)
        {
            quint32 modifiers = LOWORD(msg.lParam);
            quint32 key = HIWORD(msg.lParam);
            if(m_hotkeys.contains(QPair<quint32, quint32>(key, modifiers)))
            {
                emit hotKeyPressed(key, modifiers);
            }
        }
    }
    if(m_hotkeys.count() > 0)
    {
        QMetaObject::invokeMethod(this, "testHotKeyPress",
                              Qt::QueuedConnection);
    }
}

void WindowsKey::onRegisterHotKey(quint32 key, quint32 modifiers)
{
    if(m_hotkeys.contains(QPair<quint32, quint32>(key, modifiers)))
    {
        qDebug() << "Hotkey already registered"
                 << "modifiers" << modifiers
                 << "key" << key;
        emit registeredHotKey(false, key, modifiers);
        return;
    }
    if (!RegisterHotKey(
        NULL,
        m_hotkeys.count(),
        modifiers/*MOD_ALT*//* | MOD_NOREPEAT*/,
        key/*0x42*/))  //0x42 is 'b'
    {
        qDebug() << "Hotkey registeration failed"
                 << "modifiers" << modifiers
                 << "key" << key;
        emit registeredHotKey(false, key, modifiers);
        return;
    }
    qDebug() << "Hotkey registered"
             << "idx" << m_hotkeys.count()
             << "modifiers" << modifiers
             << "key" << key;
    m_hotkeys.append(QPair<quint32, quint32>(key, modifiers));
    emit registeredHotKey(true, key, modifiers);

    if(m_hotkeys.count() == 1)
    {
        QMetaObject::invokeMethod(this, "testHotKeyPress",
                                  Qt::QueuedConnection);
    }
    return;
}

void WindowsKey::onUnRegisterHotKey(quint32 key, quint32 modifiers)
{
    int idx = m_hotkeys.indexOf(QPair<quint32, quint32>(key, modifiers));
    if(idx == -1)
    {
        qDebug() << "Hotkey not registered";
        return;
    }
    UnregisterHotKey(NULL, idx);
    m_hotkeys.removeAt(idx);
}
