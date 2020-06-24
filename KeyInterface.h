#pragma once

#include <QImage>
#include <QMap>
#include <QPair>

struct HotKey
{
    enum Action
    {
        None,
        Record,
        ScreenShot,
    };
    Action m_action;
    quint32 m_modifiers;
    quint32 m_key;
    QString m_name;

    HotKey(Action action, quint32 modifiers, quint32 key, const QString &name)
        : m_action(action)
        , m_modifiers(modifiers)
        , m_key(key)
        , m_name(name)
    {}
    HotKey()
        : m_action(None)
        , m_modifiers(-1)
        , m_key(-1)
    {}
};

class KeyInterface : public QObject
{
    Q_OBJECT
public:
    KeyInterface(QObject *parent);
    virtual ~KeyInterface();

protected slots:
    virtual void testHotKeyPress() = 0;
    virtual void onRegisterHotKey(quint32 key, quint32 modifiers) = 0;
    virtual void onUnRegisterHotKey(quint32 key, quint32 modifiers) = 0;
signals:
    void registerHotKey(quint32 key, quint32 modifiers) const;
    void registeredHotKey(bool success, quint32 key, quint32 modifiers) const;
    void unRegisterHotKey(quint32 key, quint32 modifiers) const;
    void hotKeyPressed(quint32 key, quint32 modifiers) const;

protected:
};
