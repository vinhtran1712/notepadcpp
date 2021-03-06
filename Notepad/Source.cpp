#include <windows.h>
#include <shobjidl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <winnt.h>
#pragma comment(lib, "comctl32.lib")
#define IDM_EDUNDO 10101
#define IDM_EDCUT 10102
#define IDM_EDCOPY 10103
#define IDM_EDPASTE 10104
#define IDM_EDDEL 10105
#define IDM_ABOUT 300
#define IDM_NEW 6002
#define IDM_SAVE 6003
#define IDM_OPEN 6001
#define IDC_MAIN_TOOL 102
#define IDC_MAIN_STATUS 103
#define IDC_MAIN_EDIT 101
#define IDI_ICON      101

#define IDC_STATUSBAR 1000
#ifndef UNICODE

#define UNICODE
#endif 
HWND g_hEdit;
HFONT g_hFont;
HWND g_hwnd;
HMENU hMenu;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DoOpenFile(HWND);
void AddMenus(HWND);
void DoSaveFile(HWND);
void DoFileOpen(HWND);
void DoFileSave(HWND);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR  lpszArgument, int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"Note Pad";
	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg;
	InitCommonControls();
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbSize = sizeof(WNDCLASSEX);

	//default icon and mouse-pointer
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(102));
	wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = MAKEINTRESOURCE(202);
	wc.cbClsExtra = 0;                      // No extra bytes after the window class 
	wc.cbWndExtra = 0;                      // structure or the window instance 
	// default background color
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	if (!RegisterClassEx(&wc)) {
		return 0;
	}

	hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Note Pad",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		0,
		hInstance,
		NULL
	);
	if (!hwnd) {
		MessageBox(NULL, L"Cant create the window...\nExit Now", L"ERROR", MB_OK | MB_ICONERROR);
		return 0;
	}
	g_hwnd = hwnd;
	ShowWindow(hwnd, nCmdShow);

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(203));

	bool done = false;
	while (!done) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				done = true;
			}
			else if (!TranslateAccelerator(g_hwnd, hAccel, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	// The program return-value is 0 - The value that PostQuitMessage() gave
	return msg.wParam;
}
const int ImageListID = 0;
const int numButtons = 3;
const int bitmapSize = 16;
HIMAGELIST g_hImageList = NULL;
const DWORD buttonStyles = BTNS_AUTOSIZE;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hEdit;
	int col = 1;
	int ln = 1;
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		if (MessageBox(hwnd, L"Do you want to Quit", L"Note Pad", MB_OKCANCEL) == IDOK) {
			DestroyWindow(hwnd);
		}
		return 0;
	case WM_CREATE:
	{

		HFONT hFont;
		//For Edit Control...
		hEdit = CreateWindowEx(
			0, L"EDIT",   // predefined class 
			NULL,         // no window title 
			WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			100, 100, 800, 400,   // set size in WM_SIZE message 
			hwnd,         // parent window 
			(HMENU)IDC_MAIN_EDIT,   // edit control ID 
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
			NULL);        // pointer not needed 
		if (hEdit == NULL)
		{
			MessageBox(g_hwnd, L"Could not Create Edit control!!", L"Error", MB_OK | MB_ICONERROR);
			PostQuitMessage(0);
		}

		hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));


		g_hEdit = hEdit;
		g_hFont = hFont;

		RECT rcClient;
		GetClientRect(g_hwnd, &rcClient);
		SetWindowPos(g_hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
		//Create Toolbar
		HWND hToolbar = CreateWindowEx(
			0, TOOLBARCLASSNAME, NULL, WS_CHILD | TBSTYLE_WRAPABLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_MAIN_TOOL, GetModuleHandle(NULL), NULL
		);
		if (hToolbar == NULL) {
			MessageBox(g_hwnd, L"Cant create ToolBar", L"ToolBar Error", MB_OK | MB_ICONERROR);
			return NULL;
		}
		//Create the image list
		g_hImageList = ImageList_Create(bitmapSize, bitmapSize, ILC_COLOR16 | ILC_MASK, numButtons, 0);

		//Set the image list
		SendMessage(hToolbar, TB_SETIMAGELIST, (WPARAM)ImageListID, (LPARAM)g_hImageList);
		//Load button image
		SendMessage(hToolbar, TB_LOADIMAGES, (WPARAM)IDB_STD_SMALL_COLOR, (LPARAM)HINST_COMMCTRL);

		//Load text from resource 
		// In the string table, the text for all buttons is a single entry that 
		// appears as "~New~Open~Save~~". The separator character is arbitrary, 
		// but it must appear as the first character of the string. The message 
		// returns the index of the first item, and the items are numbered 
		// consecutively.

		int iNew = SendMessage(hToolbar, TB_ADDSTRING, (WPARAM)GetModuleHandle(NULL), (LPARAM)"&New...");

		// Initialize button info.
		// IDM_NEW, IDM_OPEN, and IDM_SAVE are application-defined command constants.
		TBBUTTON tbButtons[numButtons] =
		{
			{ MAKELONG(STD_FILENEW,  ImageListID), IDM_NEW,  TBSTATE_ENABLED, buttonStyles, {0}, 0, iNew },
			{ MAKELONG(STD_FILEOPEN, ImageListID), IDM_OPEN, TBSTATE_ENABLED, buttonStyles, {0}, 0, iNew + 1},
			{ MAKELONG(STD_FILESAVE, ImageListID), IDM_SAVE, 0, buttonStyles, {0}, 0, iNew + 2}
		};

		// Add buttons.
		SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		SendMessage(hToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);

		// Resize the toolbar, and then show it.
		SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
		ShowWindow(hToolbar, TRUE);
		AddMenus(hwnd);
	}

	break;
	case WM_SIZE:
		HWND hTool;
		RECT rcTool;
		int iToolHeight;
		int iEditHeight;
		RECT rcClient;
		// Size toolbar and get height
		hTool = GetDlgItem(hwnd, IDC_MAIN_TOOL);
		SendMessage(hTool, TB_AUTOSIZE, 0, 0);
		GetWindowRect(hTool, &rcTool);
		iToolHeight = rcTool.bottom - rcTool.top;
		//Calculate the remaining for the edit control
		GetClientRect(hwnd, &rcClient);
		iEditHeight = rcClient.bottom - iToolHeight;
		hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
		SetWindowPos(hEdit, NULL, 0, iToolHeight, rcClient.right, iEditHeight, SWP_NOZORDER);


		break;
	case WM_COMMAND:
		switch (wParam) {
		case IDM_EDUNDO:
			if (SendMessage(hEdit, EM_CANUNDO, 0, 0)) {
				SendMessage(hEdit, WM_UNDO, 0, 0);
			}
			else {
				MessageBox(hEdit, L"Nothing to undo", L"Undo Notification", MB_OK);
			}
			break;
		case IDM_EDCUT:
			SendMessage(hEdit, WM_CUT, 0, 0);
			break;

		case IDM_EDCOPY:
			SendMessage(hEdit, WM_COPY, 0, 0);
			break;

		case IDM_EDPASTE:
			SendMessage(hEdit, WM_PASTE, 0, 0);
			break;

		case IDM_EDDEL:
			SendMessage(hEdit, WM_CLEAR, 0, 0);
			break;
		case 4:
			DestroyWindow(hwnd);
			break;
		case 2:
			DoFileOpen(hwnd);
			break;
		case 1:
			SetDlgItemText(hwnd, IDC_MAIN_EDIT, L"");
			break;
		case 3:
			DoFileSave(hwnd);
			break;
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);


}

void AddMenus(HWND hwnd) {
	hMenu = CreateMenu();
	HMENU hFileMenu = CreateMenu();
	AppendMenu(hFileMenu, MF_STRING, 1, L"New");
	AppendMenu(hFileMenu, MF_STRING, 2, L"Open");
	AppendMenu(hFileMenu, MF_STRING, 3, L"Save");
	AppendMenu(hFileMenu, MF_STRING, 4, L"Exit");
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

	SetMenu(hwnd, hMenu);
}


void LoadTextFileToEdit(HWND hEdit, LPWSTR pszFileName)
{
	HANDLE hFile;

	hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize;

		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize != 0xFFFFFFFF)
		{
			LPWSTR pszFileText;

			pszFileText = (LPWSTR)GlobalAlloc(GPTR, 2 * (dwFileSize + 1));
			if (pszFileText != NULL)
			{
				DWORD dwRead;

				if (ReadFile(hFile, pszFileText, 2 * dwFileSize, &dwRead, NULL))
				{
					pszFileText[dwFileSize] = L'\0'; // Add null terminator
					SetWindowText(hEdit, pszFileText);

				}
				GlobalFree(pszFileText);
			}
		}
		CloseHandle(hFile);
	}

}

void SaveTextFileFromEdit(HWND hEdit, LPWSTR pszFileName)
{
	HANDLE hFile;


	hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwTextLength;

		dwTextLength = GetWindowTextLength(hEdit);
		if (dwTextLength > 0)
		{
			LPWSTR pszText;
			DWORD dwBufferSize = dwTextLength + 1;

			pszText = (LPWSTR)GlobalAlloc(GPTR, 2 * dwBufferSize);
			if (pszText != NULL)
			{
				if (GetWindowTextW(hEdit, pszText, dwBufferSize))
				{
					DWORD dwWritten;

					WriteFile(hFile, pszText, 2 * dwTextLength, &dwWritten, NULL);

				}
				GlobalFree(pszText);
			}
		}
		CloseHandle(hFile);
	}

}

void DoFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = (LPWSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"txt";

	if (GetOpenFileName(&ofn))
	{
		HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
		LoadTextFileToEdit(hEdit, (LPWSTR)szFileName);

	}
}

void DoFileSave(HWND hwnd)
{
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = (LPWSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = L"txt";
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
		SaveTextFileFromEdit(hEdit, (LPWSTR)szFileName);

	}
}