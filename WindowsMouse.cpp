#include "WindowsMouse.h"

#include <QDebug>
#include <QPixmap>
#include <windows.h>

WindowsMouse::WindowsMouse(QObject *parent)
    : MouseInterface(parent)
{
}

QImage WindowsMouse::getMouseCursor(QPoint &pos) const
{
    QImage cursor;
    // Get information about the global cursor.
    CURSORINFO ci;
    ci.cbSize = sizeof(ci);
    if(!GetCursorInfo(&ci))
    {
        return cursor;
    }
    pos = QPoint(ci.ptScreenPos.x, ci.ptScreenPos.y);

    ICONINFO icon_info;
    if(!GetIconInfo(ci.hCursor, &icon_info))
    {
        return cursor;
    }

    // Get your device contexts.
    HDC hdcScreen = GetDC(NULL);
    if(!hdcScreen)
    {
        return cursor;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if(!hdcMem)
    {
        ReleaseDC(NULL, hdcScreen);
        return cursor;
    }

    unsigned int width = 256;
    unsigned int height = 256;

    // Create the bitmap to use as a canvas.
    HBITMAP hbmCanvas = CreateCompatibleBitmap(hdcScreen, width, height);
    if(!hbmCanvas)
    {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return cursor;
    }

    // Select the bitmap into the device context.
    HGDIOBJ hbmOld = SelectObject(hdcMem, hbmCanvas);
    if(!hbmOld)
    {
        DeleteObject(hbmCanvas);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return cursor;
    }

    // Draw the cursor into the canvas.
    if(!DrawIcon(hdcMem, 0, 0, ci.hCursor))
    {
        DeleteObject(hbmCanvas);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return cursor;
    }

    BITMAP bitmap;
    if(GetObject(icon_info.hbmMask, sizeof(BITMAP), &bitmap) != 0)
    {
        cursor = QImage(bitmap.bmWidth, bitmap.bmHeight, QImage::Format_ARGB32);
        for(int i = 0, m = bitmap.bmWidth - 1; i < bitmap.bmWidth; i++, m--)
        {
            unsigned int *argb_array = (unsigned int*)cursor.scanLine(i);
            for(int j = 0, n = bitmap.bmHeight - 1; j < bitmap.bmHeight; j++, n--)
            {
                COLORREF color = GetPixel(hdcMem, m, j);//0x00bbggrr
                unsigned int red = color & 0xff;
                unsigned int green = (color >> 8) & 0xff;
                unsigned int blue = (color >> 16) & 0xff;
                unsigned int alpha = (color==0) ? 0 : 0xff000000;
                argb_array[j] = (alpha) | (red << 16) | (green << 8) | (blue);
            }
        }
        QMatrix rm;
        rm.rotate(90);
        QPixmap pixmap = QPixmap::fromImage(cursor);
        pixmap = pixmap.transformed(rm);
        cursor = pixmap.toImage();
    }

    // Clean up after yourself.
    DeleteObject(icon_info.hbmMask);
    DeleteObject(icon_info.hbmColor);
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmCanvas);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return cursor;
}
