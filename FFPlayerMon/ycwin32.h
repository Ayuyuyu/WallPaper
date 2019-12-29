// 
// Win32 Dialog ����

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
		COLOR_GRAY = 0xF0F0F0,		//��
		COLOR_WHITE = 0xFFFFFF,		//��
		COLOR_BALCK = 0	     		//��
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
	// ct�Ի��������
	class ctDialog
	{
	public:
		ctDialog();
		~ctDialog();

		// �Ի���callback
		LRESULT CALLBACK mainWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		// �����Ի������
		bool createMainDialog(int xWidth, int xHeight,
			COLORREF xbgColor = COLOR_GRAY, COLORREF xforeColor = COLOR_WHITE,
			RECT xforeRect = { 0 }, COLORREF xfontColor = COLOR_BALCK);
		// ��ʾ�Ի���
		// �������� ֱ���Ի��򱻹ر�
		void showMainDialog();

		// chooseFolders -> ѡ���ļ��н��� -> ѡ����ֱ�ӷ���partNameEdit�� 
		String chooseFolders();
		String chooseFile();
		// ����ɾ���ؼ�
		// CreateWindow@param : className , windowName, x ,y ,width, height (partType = partType | WS_VISIBLE | WS_CHILD )
		// partName@param provide for deletePart()
		// proc == callback : function<int CALLBACK( DWORD )>
		// ext : bCaptionFont == true ? fontsize=16 : fontsize=system-default
		HWND createPart(String className, String windowName, DWORD partType,
			int x, int y, int width, int height, String partName,
			CommandCallback proc = nullptr, int fontSize = 0);
		// partName == "*" ʱΪɾ������
		bool deletePart(String partName);
		void deleteAllPart();
		// ֱ����յ�ǰ���
		void clearDlg();

		//
		// ��ȡ�������Ŀؼ����
		HWND getWnd(String partName);

		//
		// �����ؼ�
		// ����ʱ����partNameʱ�ſ���ɾ��ָ���Ŀؼ�  
		// (��ȻҲ����ʹ��Ĭ�ϵ�partname == "static","button"...)
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
		// ���ؼ� (ע��:�����Ŀؼ����ᱣ���allcreated)
		//
		// ɾ���Ѿ�����ȥ�Ŀؼ� (����drawxxxx��ȥ��)
		// 
		template <typename T> void eraseDrewPart(T& infos, String partName);
		void eraseDrewAll();

		// draw line (�����¼���wm_paint�ﻭ��)
		// line =>  sx,sy -> ex,ey
		void drawLine(int sx, int sy, int ex, int ey, COLORREF col = RGB(0, 0, 0), String partName = TEXT("line"));
		void eraseLine(String partName);

		// draw images(bmp)
		void drawBmp(String filename, int cx, int cy, int width, int height, String partName = TEXT("bmp"));
		void drawBmp(int resID, int cx, int cy, int width, int height, String partName = TEXT("bmp"));
		void eraseBmp(String partName);

		// ���Ի�����ʽ
		void setTitle(String title);
		void setbgcolor(COLORREF col);
		void setFontColor(COLORREF col);
		void setForecolor(COLORREF col, RECT foreRt = { 0 });

		//
		void OnTrayIcon(HWND hWnd, LPARAM lParam);
		// static�ص� (��ת)
		static LRESULT CALLBACK staticWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	public:
		HWND hMainDlg;
		HINSTANCE appInstance;
		RECT hMainDlgRect;

	private:
		COLORREF bgColor, fontColor, foreColor;
		RECT forecolorRect;
		// ��¼����bmpImages����Ϣ
		list<BmpInfo> bmpInfos;
		// ��¼����LineTo����Ϣ
		list<LineInfo> lineInfos;
		// ��¼��ǰ�Ի����е����������Ϣ
		list<PartInfo> allcreated;
		// record all part's callback
		map<int, CommandCallback> regCommandMap;

		// static����:
		// ��¼��ǰ���������е����,�Թ����ö�Ӧ��proc
		static list<ctDialogInfo> ctDialogInfos;
	};
}


#endif#pragma once
