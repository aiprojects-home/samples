// SFXPlayer.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SFXPlayer.h"
#include "XSoundBank.h"
#include "XException.h"
#include "XEFM.h"
#include "XEFMReader.h"
#include "XAudioCore.h"
#include "XAudioPlayer.h"

#include <string>
#include <memory>
#include <vector>

#include "XBitMatrix.h"
#include "XKeyGenerator.h"
#include "XWMADecoder.h"

#include "XDecoderManager.h"

#include "XSoundBankParser.h"

#include <cwctype>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

XSoundBank bank(50 * 1024); // 50kb - 1 voice
XAudioPlayer player;


class my_handler : public IXAudioPlayerCallback
{
	virtual void OnStreamingEnd(uint16_t id)
	{
		XAux::PrintDebug(L"Stream = '%d' stopped\n", id);
	}

	virtual void OnError(uint16_t id, XException e)
	{
		e.Dump();
	}
};

int solutionNumberOfDiscIntersections(const std::vector<int>& A)
{
	using disc = std::pair<long long, long long>;

	size_t len = A.size();
	if (!len)
	{
		return 0;
	}

	std::vector<disc> intervals;
	
	for (size_t i = 0; i < len; ++i)
	{
		intervals.push_back(std::make_pair(i - static_cast<long long>(A[i]), i + static_cast<long long>(A[i])));
	};

	std::sort(intervals.begin(), intervals.end());

	auto x = [](const disc& a, const disc& b) -> bool
	{
		return !(a.second >= b.first);
	};

	int count{ 0 };

	for (size_t i = 0; i < len - 1; ++i)
	{
		auto it = intervals.begin() + i + 1;
		auto xr = std::upper_bound(it, intervals.end(), intervals[i], x);

		count += (std::distance(it - 1, xr) - 1);

		if (count > 10000000)
			return -1;
	};

	return count;
}

int _solutionNumberOfDiscIntersections(std::vector<int>& A)
{
	using disc = std::pair<long long, long long>;

	int len = A.size();
	if (!len)
	{
		return 0;
	};

	std::vector<disc> intervals;
	
	for (int i = 0; i < len; ++i)
	{
		intervals.push_back(std::make_pair(i - (long long)A[i], i + (long long)A[i]));
	};
	
	std::sort(intervals.begin(), intervals.end());
	
	int l, h, m, count = 0;

	for (int i = 0; i < len - 1; ++i)
	{
		l = i + 1;
		h = len - 1;

		while (l <= h)
		{
			m = l + (h - l) / 2;
		
			if (intervals[i].second >= intervals[m].first)
			{
				l = m + 1;
			}
			else
			{
				h = m - 1;
			};
		}

		count += l - 1 - i;
		
		if (count > 10000000)
		{
			return -1;
		};
	}
	
	return count;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


	// DISCS...
/*	std::vector<int> d({ 1,5,2,1,5,5,5,5,3,2,1,2,4,0 , 1, 2, 3, 55, 66, 12, 1, 0, 0, 12, 11, 2});

	int n1 = _solutionNumberOfDiscIntersections(d);
	int n2 = solutionNumberOfDiscIntersections(d);
*/
	//
	
	// TODO: Place code here.


	::CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED);

	my_handler h;

	try
	{
		WAVEFORMATEX wfex;
		std::unique_ptr<BYTE[]> spBuffer;
		uint32_t size;
		BOOL bOK;

		XAudioCore::Current().Init();

		//XEFM::Current().CreateStorage(L"e:\\_1", L"e:\\storage.e");
		XEFM::Current().AssignFile(L"e:\\_1\\storage.e");
		XEFM::Current().SetExtendedMode(true);

		bank.AssignFile(L"\\bank_v2.txt");

		player.AttachSoundBank(bank);
		player.SetMaxVoices(2);
		player.SetMaxStreamVoices(2);
		player.SetCallback(&h);
		player.Start();

	}
	catch (const XException& e)
	{
		//const wchar_t* error = e.GetError();
		e.Dump();

		int a = 0;
	}
	catch (...)
	{
		int a = 0;
	}

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SFXPLAYER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SFXPLAYER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	player.Stop();

	XAudioCore::Current().Done();

	::CoUninitialize();

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SFXPLAYER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SFXPLAYER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_CHAR:
		{
		if (wParam == 49)
		{
			// '1'
			player.PlaySimple(100);
			int a = 0;
		}
		else if (wParam == 50)
		{
			// '2'
			player.PlaySimple(101);
		}
		else if (wParam == 51)
		{
			// '3'
			player.PlaySimple(200);
		}
		else if (wParam == 48)
		{
			// '0'
			player.PlayStream(1);
		}
		else if (wParam == 57)
		{
		// '9'
		player.PlayStream(2);
		}
		else if (wParam == 56)
		{
			// '8'
			player.StopStream(1);
			player.StopStream(2);
		}
		else
		break;
		}
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
