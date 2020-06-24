#include "InterfaceFactory.h"

#if defined(WIN32) || defined(Win64)
#include <windows.h>
#include "WAVEIN.h"
#include "WindowsKey.h"
#include "WindowsMouse.h"
#include "WindowsWindowEnumerator.h"
#elif defined Linux
#include "AlsaIn.h"
#include "X11Mouse.h"
#include "X11Key.h"
#include "X11WindowEnumerator.h"
#elif Darwin
#endif


InterfaceFactory::InterfaceFactory()
{
}

AudioInInterface *InterfaceFactory::createAudioInterface()
{
#if defined(Win32) || defined(Win64)
    return new WAVEIN;
#elif defined(Linux)
    return new AlsaIn;
#else
    return new NullAudioIn;
#endif
}

MouseInterface* InterfaceFactory::createMouseInterface(QObject *parent)
{
#if defined(Win32) || defined(Win64)
    return new WindowsMouse(parent);
#elif defined(Linux)
    return new X11Mouse(parent);
#else
    return new NullMouse(parent);
#endif
}

KeyInterface* InterfaceFactory::createKeyInterface(QObject *parent)
{
#if defined(Win32) || defined(Win64)
    return new WindowsKey(parent);
#elif defined(Linux)
    return new X11Key(parent);
#else
    return new NullKey(parent);
#endif
}

WindowEnumeratorInterface* InterfaceFactory::createWindowEnumeratorInterface(QObject *parent)
{
#if defined(Win32) || defined(Win64)
    return new WindowsWindowEnumerator(parent);
#elif defined(Linux)
    return new X11WindowEnumerator(parent);
#else
    return new NullKey(parent);
#endif
}


