#include "display.h"
#include <stdexcept>
#include <windows.h>
#include <iostream>
#include <VersionHelpers.h>

template <typename T>
inline T check_SDL(T value, const std::string &message) {
	if (!value) {
		throw std::runtime_error{"SDL " + message};
	} else {
		return value;
	}
}

SDL::SDL() {
	check_SDL(!SDL_Init(SDL_INIT_AUDIO |SDL_INIT_VIDEO | SDL_INIT_TIMER), "init");
}

SDL::~SDL() {
	SDL_Quit();
}



extern HWND s_hWorkerWnd;

Display::Display(const unsigned width, const unsigned height):
	//window_{check_SDL(SDL_CreateWindow(
	//	"player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	//	width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN),
	//	"window"), SDL_DestroyWindow},
	window_{ check_SDL(SDL_CreateWindowFrom(
		//(void*)::FindWindowEx(nullptr, nullptr, nullptr, "FolderView")
		(void*)GetDesktopHanle()
	),"windows"), SDL_DestroyWindow},
	renderer_{check_SDL(SDL_CreateRenderer(
		window_.get(), -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
		"renderer"), SDL_DestroyRenderer},
	texture_{check_SDL(SDL_CreateTexture(
		renderer_.get(), SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		width,height), "renderer"), SDL_DestroyTexture}

{
	//std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window_1 = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>( check_SDL(SDL_CreateWindowFrom((void*)s_hWorkerWnd),"windows"), SDL_DestroyWindow );
	//renderer_ = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>( check_SDL(SDL_CreateRenderer(
	//	window_.get(), -1,
	//	SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
	//	"renderer"), SDL_DestroyRenderer );
	//texture_ = std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)>( check_SDL(SDL_CreateTexture(
	//	renderer_.get(), SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
	//	width, height), "renderer"), SDL_DestroyTexture );
	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	//SDL_RenderSetLogicalSize(renderer_.get(), width, height);

	SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
	SDL_RenderClear(renderer_.get());
	SDL_RenderPresent(renderer_.get());
}
//sdl 更新
void Display::refresh(
	std::array<uint8_t*, 3> planes, std::array<size_t, 3> pitches) {
	check_SDL(!SDL_UpdateYUVTexture(
		texture_.get(), nullptr,
		planes[0], pitches[0],
		planes[1], pitches[1],
		planes[2], pitches[2]), "texture update");
	SDL_RenderClear(renderer_.get());
	SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, nullptr);
	SDL_RenderPresent(renderer_.get());
}


BOOL CALLBACK EnumWindowProcFindDesktopWindow(HWND hTop, LPARAM lparam)
{
	// 查找 SHELLDLL_DefView 窗体句柄
	// 存在多个WorkerW窗体，只有底下存在SHELLDLL_DefView的才是
	HWND hWndShl = ::FindWindowEx(
		hTop, NULL, TEXT("SHELLDLL_DefView"), nullptr);
	if (hWndShl != nullptr)
	{
		//s_hWorkerWnd = hTop;
		s_hWorkerWnd = FindWindowEx(0, hTop, TEXT("WorkerW"), 0);
	}
	return true;

	//return s_hWorkerWnd == false;
}

HWND Display::GetDesktopHanle() {
	if (IsWindows8OrGreater()){
		DWORD_PTR result = 0;
		HWND windowHandle = FindWindow(TEXT("Progman"), nullptr);
		////使用 0x3e8 命令分割出两个 WorkerW
		SendMessageTimeout(windowHandle, 0x052c, 0, 0, SMTO_NORMAL, 1000, &result);
		//便利
		EnumWindows(EnumWindowProcFindDesktopWindow, (LPARAM)nullptr);
		if (s_hWorkerWnd == nullptr) {
			std::cerr << "Error: " << "HWDN" << std::endl;
			return nullptr;
		}
		// 显示WorkerW
		//ShowWindow(s_hWorkerWnd, SW_HIDE);
		ShowWindow(s_hWorkerWnd, SW_SHOW);
		std::cout << s_hWorkerWnd << std::endl;;
	}
	else {
		//if (hWndShl == nullptr) { return true; }
		// XP 直接查找SysListView32窗体
		// g_hWorker = ::FindWindowEx(hWndShl, nullptr, _T("SysListView32"),_T("FolderView"));
		// win7或更高版本
		// 查找 WorkerW 窗口句柄(以桌面窗口为父窗口)
		s_hWorkerWnd = ::FindWindowEx(nullptr, nullptr, TEXT("WorkerW"), nullptr);
	}

	return s_hWorkerWnd;
}

void Display::input() {
	if (SDL_PollEvent(&event_)) {
		switch (event_.type) {
		case SDL_KEYUP:
			switch (event_.key.keysym.sym) {
			case SDLK_ESCAPE:
				quit_ = true;
				break;
			case SDLK_SPACE:
				play_ = !play_;
				break;
			default:
				break;
			}
			break;
		case SDL_QUIT:
			quit_ = true;
			break;
		default:
			break;
		}
	}
}

bool Display::set_quit() {
	quit_ = true;
	return quit_;
}
bool Display::get_quit() {
	return quit_;
}

bool Display::get_play() {
	return play_;
}
