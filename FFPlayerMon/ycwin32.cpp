#include "ycwin32.h"
#include <tchar.h>
#include "Resource.h"
namespace ctwin32
{
	using namespace std;

	// �����ⶨ�����ڵ�static����
	list<ctDialogInfo> ctDialog::ctDialogInfos;

	//
	// ct�Ի��������
	ctDialog::ctDialog()
	{
		appInstance = GetModuleHandle(nullptr);
		//������ PROGRESS_CLASS
		InitCommonControls();
	}
	ctDialog::~ctDialog()
	{
		regCommandMap.clear();
		allcreated.clear();
		lineInfos.clear();
		bmpInfos.clear();

		// ����Ҫ��һ���ж�: ctDialogInfos�Ƿ�ɶ�
		// �����౻����Ϊȫ�ֶ���,�ڳ����˳�ʱ��������ʱ,��static-var�����Ѿ���ɾ��
		if (ctDialogInfos.size())
		{
			// ��������ʱ���ctDialogInfos�м�¼��objectָ��
			for (auto it = ctDialogInfos.begin(); it != ctDialogInfos.end(); it++)
			{
				if (it->hWnd == hMainDlg)
				{
					ctDialogInfos.erase(it);
					break;
				}
			}
		}
	}

	void ctDialog::OnTrayIcon(HWND hWnd, LPARAM lParam)
	{

		if (lParam == WM_RBUTTONDOWN)
		{
			POINT pt;//���ڽ����������
			int menu_rtn;//���ڽ��ղ˵�ѡ���ֵ
			HMENU hmenu;
			hmenu = CreatePopupMenu();//���ɲ˵�
			AppendMenu(hmenu, MF_STRING, IDM_EXIT, TEXT("�˳�"));
			GetCursorPos(&pt);//ȡ�������
			::SetForegroundWindow(hWnd);
			//EnableMenuItem(hmenu, IDR_PAUSE, MF_GRAYED);
			menu_rtn = TrackPopupMenu(hmenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);//��ʾ�˵�����ȡѡ��ID
			if (menu_rtn == IDM_EXIT)
			{
				//do some thing;
				PostQuitMessage(0);
			}
				

		}
		else if(lParam == WM_LBUTTONDOWN)
		{
			ShowWindow(hWnd, SW_SHOW);
		}
	}

	// �Ի���callback
	LRESULT CALLBACK ctDialog::mainWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_COMMAND:
		{
			auto callback = [&](HWND hWnd, DWORD windowId)->int {
				auto it = regCommandMap.find(windowId);
				if (it != regCommandMap.end() && it->second)
					return it->second(hWnd, windowId);
				return -1;
			};

			return callback(hWnd, LOWORD(wParam));
		}

		case WM_CTLCOLORSTATIC: //bg-static
		{
			TCHAR tmp[MAX_PATH] = {0};
			GetWindowText((HWND)lParam, tmp, 200);

			RECT rt;
			COLORREF tmpbg = bgColor;
			GetWindowRect((HWND)lParam, &rt);
			POINT pt = { rt.left,rt.top };
			ScreenToClient(hWnd, &pt);
			if (PtInRect(&forecolorRect, { pt.x,pt.y }))
				tmpbg = foreColor;
			SetTextColor((HDC)wParam, fontColor);
			SetBkColor((HDC)wParam, tmpbg);
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (INT_PTR)CreateSolidBrush(tmpbg);
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rect;
			GetClientRect(hWnd, &rect);
			hdc = BeginPaint(hWnd, &ps);

			// brush color
			auto brushWindow = [&](COLORREF color, RECT rt) {
				HBRUSH brush = CreateSolidBrush(color);		//����
				HPEN pen = CreatePen(PS_NULL, 0, 0);			//����
				SelectObject(hdc, brush);
				SelectObject(hdc, pen);
				Rectangle(hdc, rt.left, rt.top, rt.right, rt.bottom);
				DeleteObject(pen);
				DeleteObject(brush);
			};
			// brush background-color
			brushWindow(bgColor, rect);
			// brush forecolor
			brushWindow(foreColor, forecolorRect);

			// Draw Lines
			auto drawline = [&](COLORREF col, int sx, int sy, int ex, int ey) {
				HPEN pen = CreatePen(PS_SOLID, 0, col);			//����
				SelectObject(hdc, pen);
				MoveToEx(hdc, sx, sy, NULL);
				LineTo(hdc, ex, ey);
				DeleteObject(pen);
			};
			for (auto& it : lineInfos)
				drawline(it.color, it.lstart.x, it.lstart.y, it.lend.x, it.lend.y);

			// show bmp
			auto drawbmp = [&](String& filename, int cx, int cy, int width, int height, int resid = 0) {
				//�������Ҫ��Ч�ʸߵĻ� Ҫ��ǰload
				HBITMAP hbmp;
				if (!resid)
				{
					hbmp = (HBITMAP)LoadImage(NULL, filename.c_str(),
						IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
				}
				else
				{
					hbmp = (HBITMAP)LoadImage(appInstance, MAKEINTRESOURCE(resid),
						IMAGE_BITMAP, 0, 0, LR_SHARED | LR_CREATEDIBSECTION);
				}
				HDC hMemDC = CreateCompatibleDC(hdc);
				SelectObject(hMemDC, hbmp);
				BitBlt(hdc, cx, cy, width, height, hMemDC, 0, 0, SRCCOPY);
				DeleteObject(hbmp);
				DeleteObject(hMemDC);
			};
			for (auto& it : bmpInfos)
				drawbmp(it.filename, it.cx, it.cy, it.width, it.height, it.resid);

			EndPaint(hWnd, &ps);
			return FALSE;
		}
		case WM_CLOSE:
		{
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_ICON_MSG:
		{
			OnTrayIcon(hWnd, lParam);
		}

		default:
			// ��ȥ����defwndproc
			return -1;
		}
		return 0;
	}
	// ��ʼ���Ի������
	bool ctDialog::createMainDialog(int xWidth, int xHeight,
		COLORREF xbgColor, COLORREF xforeColor,
		RECT xforeRect, COLORREF xfontColor)
	{
		// constants set
		const auto szClass = TEXT("ycWizard");

		// param set
		bgColor = xbgColor;
		fontColor = xfontColor;
		foreColor = xforeColor;
		forecolorRect = xforeRect;
		hMainDlgRect = { 0,0,xWidth,xHeight };

		// register class
		WNDCLASSEX wcex = { 0 };
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = staticWndCallback;	//��ת������static����
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = appInstance;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszClassName = szClass;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
		wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDC_MYICON));
		RegisterClassExW(&wcex);

		// create main window
		HWND hWnd = CreateWindowW(szClass, TEXT("ycDialog"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
			CW_USEDEFAULT, 0, xWidth, xHeight, nullptr, nullptr, appInstance, nullptr);
		//���ô���λ�þ���
		MoveWindow(hWnd, (GetSystemMetrics(SM_CXSCREEN) - xWidth) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - xHeight) / 2, xWidth, xHeight, FALSE);
		if (hWnd)
		{
			hMainDlg = hWnd;
			// ��ӵ�ctDialogInfos��¼��,�Ա���תcallback������ĺ���
			ctDialogInfos.push_back({ hWnd,this });

			ShowWindow(hWnd, 5);
			UpdateWindow(hWnd);
			return true;
		}

		return false;
	}
	// ��ʾ�Ի���
	// �������� ֱ���Ի��򱻹ر�
	void ctDialog::showMainDialog()
	{
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// chooseFolders -> ѡ���ļ��н��� -> ѡ����ֱ�ӷ���partNameEdit�� 
	String ctDialog::chooseFolders()
	{
		BROWSEINFO bi = { 0 };
		LPITEMIDLIST pIDList;
		bi.hwndOwner = hMainDlg;
		bi.lpszTitle = TEXT("ѡ���ļ���");
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		pIDList = SHBrowseForFolder(&bi);
		TCHAR tmp[260] = { 0 };
		if (pIDList)
			SHGetPathFromIDList(pIDList, tmp);
		return String(tmp);
	}

	String ctDialog::chooseFile()
	{
		TCHAR file_name[MAX_PATH];//file name
		OPENFILENAME opfn;
		ZeroMemory(&opfn, sizeof(OPENFILENAME));
		opfn.lStructSize = sizeof(OPENFILENAME);//ָ������ṹ�Ĵ�С
		opfn.lpstrFilter = TEXT("�����ļ�\0*.*\0");//ָ��һ���Կ��ַ������Ĺ����ַ�����һ�����塣
		//�����е����һ���ַ������������� NULL�ַ�������
		opfn.nFilterIndex = 1; //ָ�����ļ����Ϳؼ��е�ǰѡ��Ĺ�����������
		opfn.lpstrFile = file_name; //ָ�������ʼ���ļ����༭�ؼ�ʹ�õ��ļ����Ļ���
		opfn.lpstrFile[0] = '\0'; //�������ĵ�һ���ַ�������NULL
		opfn.nMaxFile = sizeof(file_name);//ָ��lpstrFile����Ĵ�С����TCHARsΪ��λ��
		//����ANSI�汾�����ֽڵĸ���������Unicode�汾�����ַ��ĸ�����
		opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST; //OFN_FILEMUSTEXIST ָ���û�
		BOOL bOk = GetOpenFileName(&opfn);
		if (!bOk)
		{
			return String(TEXT(""));
		}
		
		return String(file_name);
	}

	// ����ɾ���ؼ�
	// CreateWindow@param : className , windowName, x ,y ,width, height (partType = partType | WS_VISIBLE | WS_CHILD )
	// partName@param provide for deletePart()
	// proc == callback : function<int CALLBACK( DWORD )>
	// ext : bCaptionFont == true ? fontsize=16 : fontsize=system-default
	HWND ctDialog::createPart(String className, String windowName, DWORD partType,
		int x, int y, int width, int height, String partName,
		CommandCallback proc, int fontSize)
	{
		// ��ȡһ��id
		auto getwindowid = [&]()->int {
			int iret = 100;  //iwIdStart=100
			for (auto it : allcreated)
			{
				if (it.windowId == iret)
					iret++;
			}
			return iret;
		};
		DWORD windowId = getwindowid();

		// ��������
		HWND hWnd = CreateWindow(className.c_str(), windowName.c_str(), WS_VISIBLE | WS_CHILD | partType,
			x, y, width, height, hMainDlg, (HMENU)windowId, appInstance, nullptr);
		if (hWnd)
		{
			HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			if (fontSize)
			{
				font = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
					OUT_TT_PRECIS, CLIP_TT_ALWAYS, DEFAULT_QUALITY,
					VARIABLE_PITCH | FF_SWISS, TEXT("΢���ź�"));
			}
			SendMessage(hWnd, WM_SETFONT, (WPARAM)font, TRUE);
			UpdateWindow(hMainDlg);

			allcreated.push_back({ partName,windowId,hWnd });
			regCommandMap.insert(pair<int, CommandCallback>(windowId, proc));
			return hWnd;
		}

		return NULL;
	}
	// partName == "*" ʱΪɾ������
	bool ctDialog::deletePart(String partName)
	{
		bool ret = false;
		for (auto it = allcreated.begin(); it != allcreated.end();)
		{
			if (it->partname == partName || partName == TEXT("*"))
			{
				// ɾ��partWindow
				DestroyWindow(it->hWnd);
				// ȡ��ע��ĺ���
				for (auto itmap = regCommandMap.begin(); itmap != regCommandMap.end(); itmap++)
				{
					if (itmap->first == it->windowId)
					{
						regCommandMap.erase(itmap++);
						break;
					}
				}
				// ɾ��������¼
				allcreated.erase(it++);
				ret = true;
				continue;
			}
			it++;
		}
		return ret;
	}
	void ctDialog::deleteAllPart()
	{
		deletePart(TEXT("*"));
	}
	// ֱ����յ�ǰ���
	void ctDialog::clearDlg()
	{
		deleteAllPart();
		eraseDrewAll();
	}

	//
	// ��ȡ�������Ŀؼ����
	HWND ctDialog::getWnd(String partName)
	{
		for (auto& it : allcreated)
		{
			if (it.partname == partName)
				return it.hWnd;
		}
		return 0;
	}

	//
	// �����ؼ�
	// ����ʱ����partNameʱ�ſ���ɾ��ָ���Ŀؼ�  
	// (��ȻҲ����ʹ��Ĭ�ϵ�partname == "static","button"...)
	//
	// text
	bool ctDialog::createText(String content, int x, int y, int width, int height,
		int isCaptionSize, String partName)
	{
		return createPart(TEXT("STATIC"), content, NULL, x, y, width, height, partName, nullptr, isCaptionSize) != 0;
	}
	// push-button
	bool ctDialog::createbutton(String content, int x, int y,
		CommandCallback proc, int width, int height, String partName)
	{
		return createPart(TEXT("button"), content, BS_PUSHBUTTON, x, y, width, height, partName, proc) != 0;
	}
	// check box
	bool ctDialog::createCheckbox(String content, int x, int y,
		CommandCallback proc, int width, int height, String partName)
	{
		HWND hc = createPart(TEXT("button"), content, BS_AUTOCHECKBOX, x, y, width, height, partName, proc);
		SendMessage(hc, BM_SETCHECK, 1, 0);
		return hc != 0;
	}
	// edit
	// @partType = WS_BORDER
	bool ctDialog::createEdit(int x, int y, int width, int height,
		String partName, String defaultContent, DWORD partType)
	{
		return createPart(TEXT("EDIT"), defaultContent, partType, x, y, width, height, partName) != 0;
	}
	bool ctDialog::setText(String partName, String text)
	{
		HWND hedit = getWnd(partName);
		if (hedit)
		{
			SetWindowText(hedit, text.c_str());
			return true;
		}
		return false;
	}
	String ctDialog::getEditText(String partName)
	{
		HWND hedit = getWnd(partName);
		if (hedit)
		{
			TCHAR tmp[1024] = {0};
			GetWindowText(hedit, tmp, 500);
			return String(tmp);
		}
		return TEXT("");
	}
	// progress bar
	bool ctDialog::createProgress(int x, int y, String partName, int defaultRange, int width, int height)
	{
		HWND hp = createPart(TEXT("msctls_progress32"), TEXT("progress"), NULL, x, y, width, height, partName, nullptr);
		SendMessage(hp, PBM_SETRANGE, 0, MAKELONG(0, defaultRange));
		SendMessage(hp, PBM_SETPOS, 0, 0);
		return hp != 0;
	}
	void ctDialog::setProgressPos(String partName, int xpos)
	{
		SendMessage(getWnd(partName), PBM_SETPOS, (WPARAM)xpos, 0);
	}

	//
	// ���ؼ� (ע��:�����Ŀؼ����ᱣ���allcreated)
	//
	// ɾ���Ѿ�����ȥ�Ŀؼ� (����drawxxxx��ȥ��)
	// 
	template <typename T>
	void ctDialog::eraseDrewPart(T& infos, String partName)
	{
		for (auto it = infos.begin(); it != infos.end(); it++)
		{
			if (it->partname == partName)
			{
				infos.erase(it);
				InvalidateRect(hMainDlg, NULL, FALSE);
				break;
			}
		}
	}
	void ctDialog::eraseDrewAll()
	{
		lineInfos.clear();
		bmpInfos.clear();
		InvalidateRect(hMainDlg, NULL, FALSE);
	}

	// draw line (�����¼���wm_paint�ﻭ��)
	// line =>  sx,sy -> ex,ey
	void ctDialog::drawLine(int sx, int sy, int ex, int ey, COLORREF col, String partName)
	{
		lineInfos.push_back({ partName, col,{sx,sy},{ex,ey} });
		InvalidateRect(hMainDlg, NULL, FALSE);
	}
	void ctDialog::eraseLine(String partName)
	{
		eraseDrewPart<list<LineInfo>>(lineInfos, partName);
	}

	// draw images(bmp)
	void ctDialog::drawBmp(String filename, int cx, int cy, int width, int height, String partName)
	{
		bmpInfos.push_back({ partName, filename,cx,cy,width,height,0 });
		InvalidateRect(hMainDlg, NULL, FALSE);
	}
	void ctDialog::drawBmp(int resID, int cx, int cy, int width, int height, String partName)
	{
		bmpInfos.push_back({ partName,TEXT(""),cx,cy,width,height,resID });
		InvalidateRect(hMainDlg, NULL, FALSE);
	}
	void ctDialog::eraseBmp(String partName)
	{
		eraseDrewPart<list<BmpInfo>>(bmpInfos, partName);
	}

	// ���Ի�����ʽ
	void ctDialog::setTitle(String title)
	{
		SetWindowText(hMainDlg, title.c_str());
	}
	void ctDialog::setbgcolor(COLORREF col)
	{
		bgColor = col;
		InvalidateRect(hMainDlg, NULL, FALSE);
	}
	void ctDialog::setFontColor(COLORREF col)
	{
		fontColor = col;
		InvalidateRect(hMainDlg, NULL, FALSE);
	}
	void ctDialog::setForecolor(COLORREF col, RECT foreRt)
	{
		foreColor = col;
		forecolorRect = foreRt;
		InvalidateRect(hMainDlg, NULL, FALSE);
	}

	//
	// static�ص� (��ת)
	LRESULT CALLBACK ctDialog::staticWndCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		for (auto& it : ctDialogInfos)
		{
			if (hWnd == it.hWnd)
			{
				LRESULT lret = (it.object)->mainWndCallback(hWnd, message, wParam, lParam);
				if (lret == -1)
					break;
				return lret;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}