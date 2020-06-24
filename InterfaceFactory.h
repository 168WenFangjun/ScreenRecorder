#pragma once

class AudioInInterface;
class MouseInterface;
class KeyInterface;
class QObject;
class WindowEnumeratorInterface;
class InterfaceFactory
{
public:
    InterfaceFactory();

    static AudioInInterface* createAudioInterface();
    static MouseInterface* createMouseInterface(QObject *parent = nullptr);
    static KeyInterface* createKeyInterface(QObject *parent = nullptr);
    static WindowEnumeratorInterface* createWindowEnumeratorInterface(QObject *parent = nullptr);
};

