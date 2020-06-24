#include "X11Key.h"
#include <QDebug>
#include <QApplication>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <QtX11Extras/QX11Info>

static bool s_last_x11_error = false;
static int MyX11ErrorHandler(Display *, XErrorEvent *)
{
    qDebug() << "X11 Error";
    s_last_x11_error = true;
}

X11Key::X11Key(QObject *parent)
    : KeyInterface(parent)
    , m_dpy(XOpenDisplay(0))
    , m_registered_hotkey_count(0)
{
    XSetErrorHandler(MyX11ErrorHandler);
}

X11Key::~X11Key()
{
    XCloseDisplay(m_dpy);
}

//alternative: https://stackoverflow.com/questions/4037230/global-hotkey-with-x11-xlib
void X11Key::testHotKeyPress()
{
    XEvent      ev;

    XSelectInput(m_dpy, DefaultRootWindow(m_dpy), KeyPressMask);
    if(XPending(m_dpy))
    {
        XNextEvent(m_dpy, &ev);
        switch(ev.type)
        {
        case KeyPress:
//            qDebug() << "Hot key pressed!"
//                     << "type" << ev.type
//                     << "modifier" << ev.xkey.state
//                     << "key" << XKeycodeToKeysym(m_dpy, ev.xkey.keycode, 0);
            emit hotKeyPressed(XLookupKeysym(&ev.xkey, 0), ev.xkey.state);
            break;
        default:
//            qDebug() << "key press detected"
//                     << "type" << ev.type
//                     << "state" << ev.xkey.state
//                     << "keycode" << ev.xkey.keycode;
            break;
        }
    }

    if(m_registered_hotkey_count > 0)
    {
        QMetaObject::invokeMethod(this, "testHotKeyPress",
                          Qt::QueuedConnection);
    }
}

void X11Key::onRegisterHotKey(quint32 key, quint32 modifiers)
{
    Window      root    = DefaultRootWindow(m_dpy);

    int             keycode         = XKeysymToKeycode(m_dpy, key/*XK_K*/);
    Bool            owner_events    = False;
    int             pointer_mode    = GrabModeAsync;
    int             keyboard_mode   = GrabModeAsync;

    s_last_x11_error = false;
    XGrabKey(m_dpy, keycode, modifiers, root, owner_events, pointer_mode,
             keyboard_mode);
    qDebug() << "onRegisterHotKey" << ((s_last_x11_error) ? "error" : "success")
             << "key" << key
             << "modifiers" << modifiers;
    if(!s_last_x11_error)
    {
        m_registered_hotkey_count++;
    }
    emit registeredHotKey(!s_last_x11_error, key, modifiers);
    if(m_registered_hotkey_count == 1)
    {
        QMetaObject::invokeMethod(this, "testHotKeyPress",
                                  Qt::QueuedConnection);
    }
}

void X11Key::onUnRegisterHotKey(quint32 key, quint32 modifiers)
{
    Window      root    = DefaultRootWindow(m_dpy);
    int             keycode         = XKeysymToKeycode(m_dpy, key);

    s_last_x11_error = false;
    XUngrabKey(m_dpy, keycode, modifiers, root);
    qDebug() << "onUnRegisterHotKey" << ((s_last_x11_error) ? "error" : "success")
             << "key" << key
             << "modifiers" << modifiers;
    if(!s_last_x11_error && m_registered_hotkey_count > 0)
    {
        m_registered_hotkey_count--;
    }
}
