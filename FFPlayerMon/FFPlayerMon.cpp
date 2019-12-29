// FFPlayerMon.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "FFPlayerMon.h"
#include "ycwin32.h"
#include "player.h"
#include "strsafe.h"
#include "shellapi.h"

 //初始化对话框
ctwin32::ctDialog ctd;

TCHAR szWallpaper[MAX_PATH] = { 0 };
HWND s_hWorkerWnd = nullptr;
 
Display* c_dis;

int run_play(std::string str_path)
{
    Player play{ str_path };
    c_dis = play.display_.get();
    play();
    
    return 1;
}

int CALLBACK choose_callback(HWND hDlg, DWORD windowId)
{
    //SystemParametersInfo(SPI_GETDESKWALLPAPER, _countof(szWallpaper), (PVOID)szWallpaper, 0);
    ctwin32::String str_file = ctd.chooseFile();
    if (str_file.length() > 0)
    {
        ctd.setText(TEXT("edit"), str_file);
        //run_play(WStringToString(str_file));
        std::thread t(run_play, WStringToString(str_file));
        t.detach();

        ::EnableWindow(ctd.getWnd(TEXT("button_choose")), FALSE);
        return 1;
    }
    return 0;
}

int CALLBACK reset_callback(HWND hDlg, DWORD windowId)
{
    c_dis->set_quit();
    ::EnableWindow(ctd.getWnd(TEXT("button_choose")), TRUE);
    return 0;
}

// main dialog
void show(HINSTANCE hInstance)
{
    ctd.createMainDialog(400, 180);
    ctd.setTitle(TEXT("FFPlayerMon"));

    //背景色
    ctd.setbgcolor(RGB(240, 240, 240));
    //前景色
    RECT rt = { 0,ctd.hMainDlgRect.bottom - 60,
        ctd.hMainDlgRect.right,ctd.hMainDlgRect.bottom };
    ctd.setForecolor(RGB(35, 39, 54), rt);

    //其他组件
    ctd.createbutton(TEXT("choose"), 290, 10, choose_callback,85,30, TEXT("button_choose"));
    ctd.createbutton(TEXT("重置"), 280, 70, reset_callback, 100, 40, TEXT("button_reset"));
    ctd.setFontColor(RGB(81, 81, 81));
    ctd.createText(TEXT("选择播放使用视频..."), 10, 10, 200, 40, 25);
    ctd.createEdit(10, 42, 360, 25);
    
    //添加托盘
    NOTIFYICONDATA m_NotifyIcon = {};
    m_NotifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    m_NotifyIcon.hWnd = ctd.hMainDlg;
    m_NotifyIcon.uID = IDI_SMALL; //托盘图标
    m_NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;//自定
    m_NotifyIcon.uCallbackMessage = WM_ICON_MSG;
    m_NotifyIcon.dwInfoFlags = NIIF_RESPECT_QUIET_TIME | NIIF_NONE;
    LoadIconMetric(hInstance, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &(m_NotifyIcon.hIcon));
    Shell_NotifyIcon(NIM_ADD, &m_NotifyIcon);
    //ShowWindow(ctd.hMainDlg,SW_HIDE);
    // UI消息循环开始，线程在这里阻塞
    ctd.showMainDialog();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    show(hInstance);
    return 1;
}
