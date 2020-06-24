#include "KeyInterface.h"

KeyInterface::KeyInterface(QObject *parent)
    : QObject(parent)
{
    connect(this, &KeyInterface::registerHotKey,
            this, &KeyInterface::onRegisterHotKey,
            Qt::QueuedConnection);
    connect(this, &KeyInterface::unRegisterHotKey,
            this, &KeyInterface::onUnRegisterHotKey,
            Qt::QueuedConnection);
}

KeyInterface::~KeyInterface()
{

}
