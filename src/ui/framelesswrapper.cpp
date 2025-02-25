#include "framelesswrapper.h"

#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMdiArea>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#ifdef Q_OS_WIN
#include "util/WindowCompositionAttribute.h"
#include <windowsx.h>
#include <dwmapi.h>
#endif

#include "windowstitlebar.h"
#include "config.hpp"

FramelessWrapper::FramelessWrapper(QMainWindow *window) :
    FramelessWrapper(window, window->menuBar())
{}

FramelessWrapper::FramelessWrapper(QDialog *dialog) :
    FramelessWrapper((QWidget*)dialog)
{
    connect(dialog, &QDialog::finished, this, &FramelessWrapper::close);
}

FramelessWrapper::FramelessWrapper(QWidget *widget, QMenuBar *menuBar) :
    QMainWindow(widget->parentWidget()),
    titleBar_(new WindowsTitleBar(widget, menuBar))
{
    titleBar_->setParentWidget(this);

    auto w = new QWidget(this);
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar_);
    layout->addWidget(widget);
    w->setLayout(layout);
    setCentralWidget(w);

    updateBlur();
}

void FramelessWrapper::updateBlur()
{
#ifdef Q_OS_WIN
    //set blur
    if(auto huser = GetModuleHandle(L"user32.dll"); huser){
        auto setWindowCompositionAttribute = (pfnSetWindowCompositionAttribute)::GetProcAddress(huser, "SetWindowCompositionAttribute");
        if(setWindowCompositionAttribute){
            ACCENT_STATE as;
            if(Config().getEnableBlurBehind()){
                    as = ACCENT_ENABLE_BLURBEHIND;
            }
            else
                as = ACCENT_ENABLE_GRADIENT;
            ACCENT_POLICY accent = { as, 0x1e0, 0x000f0f0f, 0 };
            WINDOWCOMPOSITIONATTRIBDATA data;
            data.Attrib = WCA_ACCENT_POLICY;
            data.pvData = &accent;
            data.cbData = sizeof(accent);
            setWindowCompositionAttribute(::HWND(winId()), &data);
        }
    }
#endif //Q_OS_WIN
}

#ifdef Q_OS_WIN
bool FramelessWrapper::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = (MSG*)message;
    float boundaryWidth = 4;
//    qDebug() << msg->message;
    switch(msg->message){
    case WM_NCCALCSIZE:{
        NCCALCSIZE_PARAMS& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        if (params.rgrc[0].top != 0)
            params.rgrc[0].top -= 1;

        *result = WVR_REDRAW;
        return true;
    }
    case WM_NCHITTEST:{
        //support highdpi
        double dpr = this->devicePixelRatioF();
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        auto mousePos = mapFromGlobal(QPoint(x/dpr,y/dpr));
        bool left = mousePos.x() < boundaryWidth;
        bool right = mousePos.x() > width() - boundaryWidth;
        bool top = mousePos.y() < boundaryWidth;
        bool bottom = mousePos.y() > height() - boundaryWidth;
        if(left && top)
            *result = HTTOPLEFT;
        else if(right && top)
            *result = HTTOPRIGHT;
        else if(left && bottom)
            *result = HTBOTTOMLEFT;
        else if(right && bottom)
            *result = HTBOTTOMRIGHT;
        else if(left)
            *result = HTLEFT;
        else if(right)
            *result = HTRIGHT;
        else if(top)
            *result = HTTOP;
        else if(bottom)
            *result = HTBOTTOM;
        if(*result) return true;

        if (!titleBar_->rect().contains(mousePos)) return false;
        return titleBar_->hitTest(mousePos, result);
    }
    case WM_GETMINMAXINFO: {
        if (::IsZoomed(msg->hwnd)) {
            RECT frame = { 0, 0, 0, 0 };
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
            frame.left = abs(frame.left);
            frame.top = abs(frame.bottom);
            setContentsMargins(frame.left, frame.top, frame.right, frame.bottom);
            titleBar_->setMaximumed();
        } else{
            setContentsMargins(0, 0, 0, 0);
            titleBar_->setNormal();
        }
        *result = ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        break;
    }
    }
    return false;
}
#endif //Q_OS_WIN
