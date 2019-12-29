// 
// Win32 Dialog 创建

#ifndef CTWIN32DIALOG_H
#define CTWIN32DIALOG_H

#include <list>
#include <map>
#include <functional>
#include <thread>
#include <string>
#include <Shlobj.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32") 


#define WM_ICON_MSG WM_USER + 1008
namespace ctwin32
{
	using namespace std;
	#ifdef UNICODE
	#define String wstring
	#else
	#define String string
	#endif // !UNICODE
	enum Color
	{
		COLOR_GRAY = 0xF0F0F0,		//灰
		COLOR_WHITE = 0xFFFFFF,		//白
		COLOR_BALCK = 0	     		//黑
	};

#define CommandCallback	function<int CALLBACK( HWND, DWORD )>
	class ctDialog;

	struct ctDialogInfo
	{
		HWND hWnd;
		ctDialog* object;
	};
	struct PartInfo
	{
		String partname;
		DWORD windowId;
		HWND hWnd;
	};
	struct LineInfo
	{
		String partname;
		COLORREF color;
		POINT lstart, lend;
	};
	struct BmpInfo
	{
		String partname;
		String filename;
		int cx, cy;
		int width, height;
		int resid;
	};

	//////////////////////////////////////////////////////////////////////////
	// ct对话框基础类
	class ctDialog
	{
	public:
		ctDialog();
		~ctDialog();

		// 对话框callback
		LRESULT CALLBACK mainWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		// 创建对话框面板
		bool createMainDialog(int xWidth, int xHeight,
			COLORREF xbgColor = COLOR_GRAY, COLORREF xforeColor = COLOR_WHITE,
			RECT xforeRect = { 0 }, COLORREF xfontColor = COLOR_BALCK);
		// 显示对话框
		// 阻塞函数 直到对话框被关闭
		void showMainDialog();

		// chooseFolders -> 选择文件夹界面 -> 选定后直接放入partNameEdit中 
		String chooseFolders();
		String chooseFile();
		// 增加删除控件
		// CreateWindow@param : className , windowName, x ,y ,width, height (partType = partType | WS_VISIBLE | WS_CHILD )
		// partName@param provide for deletePart()
		// proc == callback : function<int CALLBACK( DWORD )>
		// ext : bCaptionFont == true ? fontsize=16 : fontsize=system-default
		HWND createPart(String className, String windowName, DWORD partType,
			int x, int y, int width, int height, String partName,
			CommandCallback proc = nullptr, int fontSize = 0);
		// partName == "*" 时为删除所有
		bool deletePart(String partName);
		void deleteAllPart();
		// 直接清空当前面板
		void clearDlg();

		//
		// 获取创建过的控件句柄
		HWND getWnd(String partName);

		//
		// 创建控件
		// 调用时带上partName时才可以删除指定的控件  
		// (当然也可以使用默认的partname == "static","button"...)
		//
		// text
		bool createText(String content, int x, int y, int width, int height,
			int isCaptionSize = 0, String partName = TEXT("static"));
		// push-button
		bool createbutton(String content, int x, int y, CommandCallback proc = nullptr,
			int width = 85, int height = 22, String partName = TEXT("button"));
		// check-button
		bool createCheckbox(String content, int x, int y, CommandCallback proc = nullptr,
			int width = 150, int height = 22, String partName = TEXT("checkbox"));
		// edit
		bool createEdit(int x, int y, int width, int height = 20,
			String partName = TEXT("edit"), String defaultContent = TEXT(""), DWORD partType = WS_BORDER | ES_MULTILINE);
		bool setText(String partName, String text);
		String getEditText(String partName);
		// progress bar
		bool createProgress(int x, int y, String partName = TEXT("progress"),
			int defaultRange = 100, int width = 400, int height = 20);
		void setProgressPos(String partName, int xpos);

		//
		// 画控件 (注意:这样的控件不会保存进allcreated)
		//
		// 删除已经画上去的控件 (仅限drawxxxx上去的)
		// 
		template <typename T> void eraseDrewPart(T& infos, String partName);
		void eraseDrewAll();

		// draw line (保存记录后从wm_paint里画的)
		// line =>  sx,sy -> ex,ey
		void drawLine(int sx, int sy, int ex, int ey, COLORREF col = RGB(0, 0, 0), String partName = TEXT("line"));
		void eraseLine(String partName);

		// draw images(bmp)
		void drawBmp(String filename, int cx, int cy, int width, int height, String partName = TEXT("bmp"));
		void drawBmp(int resID, int cx, int cy, int width, int height, String partName = TEXT("bmp"));
		void eraseBmp(String partName);

		// 主对话框样式
		void setTitle(String title);
		void setbgcolor(COLORREF col);
		void setFontColor(COLORREF col);
		void setForecolor(COLORREF col, RECT foreRt = { 0 });

		//
		void OnTrayIcon(HWND hWnd, LPARAM lParam);
		// static回调 (中转)
		static LRESULT CALLBACK staticWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	public:
		HWND hMainDlg;
		HINSTANCE appInstance;
		RECT hMainDlgRect;

	private:
		COLORREF bgColor, fontColor, foreColor;
		RECT forecolorRect;
		// 记录所有bmpImages的信息
		list<BmpInfo> bmpInfos;
		// 记录所有LineTo的信息
		list<LineInfo> lineInfos;
		// 记录当前对话框中的所有组件信息
		list<PartInfo> allcreated;
		// record all part's callback
		map<int, CommandCallback> regCommandMap;

		// static数据:
		// 记录当前创建的所有的面板,以供调用对应的proc
		static list<ctDialogInfo> ctDialogInfos;
	};
}


#endif#pragma once
