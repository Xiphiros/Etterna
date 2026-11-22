#ifndef INPUT_HANDLER_WIN32_RAWINPUT_H
#define INPUT_HANDLER_WIN32_RAWINPUT_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>
#include <windows.h>

class InputHandler_Win32_RawInput : public InputHandler
{
  public:
	InputHandler_Win32_RawInput();
	~InputHandler_Win32_RawInput() override;

	void GetDevicesAndDescriptions(
	  std::vector<InputDeviceInfo>& vDevicesOut) override;
	wchar_t DeviceButtonToChar(DeviceButton button,
							   bool bUseCurrentKeyModifiers) override;
	void Update() override;
	void WindowReset() override;

  private:
	static LRESULT CALLBACK RawInputWndProc(HWND hwnd,
											UINT msg,
											WPARAM wParam,
											LPARAM lParam);
	static int InputThread_Start(void* p);
	void InputThreadMain();

	HWND CreateSinkWindow();
	void DestroySinkWindow(HWND hwnd);
	void HandleRawInput(HRAWINPUT hRawInput,
						const std::chrono::steady_clock::time_point& now);

	RageThread m_InputThread;
	HANDLE m_hStopEvent{ nullptr };
	HANDLE m_hReadyEvent{ nullptr };
	HWND m_hMsgWnd{ nullptr };
	std::atomic<bool> m_bShutdown{ false };
	bool m_bInitialized{ false };

	std::mutex m_StateMutex;
	std::bitset<NUM_DeviceButton> m_HeldKeys;
	bool m_bFocused{ true };
	int m_iWheelRemainder{ 0 };
	std::vector<uint8_t> m_vRawBuffer;
};

#endif
