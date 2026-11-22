#include "Etterna/Globals/global.h"
#include "InputHandler_Win32_RawInput.h"
#include "InputHandler_DirectInputHelper.h"

#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/InputFilter.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "archutils/Win32/ErrorStrings.h"
#include "archutils/Win32/GraphicsWindow.h"

#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif

REGISTER_INPUT_HANDLER_CLASS2(RawInput, Win32_RawInput);

namespace {
constexpr const wchar_t* kRawInputWindowClass = L"EtternaRawInputSink";
constexpr USHORT kHidUsagePageGeneric = 0x01;
constexpr USHORT kHidUsageGenericMouse = 0x02;
constexpr USHORT kHidUsageGenericKeyboard = 0x06;
} // namespace

LRESULT CALLBACK
InputHandler_Win32_RawInput::RawInputWndProc(HWND hwnd,
											 UINT msg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	if (msg == WM_INPUT) {
		auto* pSelf = reinterpret_cast<InputHandler_Win32_RawInput*>(
		  GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (pSelf != nullptr) {
			pSelf->HandleRawInput(reinterpret_cast<HRAWINPUT>(lParam),
								  std::chrono::steady_clock::now());
		}
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND
InputHandler_Win32_RawInput::CreateSinkWindow()
{
	const HINSTANCE hInst = GetModuleHandleW(nullptr);
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = RawInputWndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = kRawInputWindowClass;
	if (RegisterClassExW(&wc) == 0 &&
		GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		Locator::getLogger()->warn(
		  "{}",
		  werr_ssprintf(GetLastError(), "RawInput: RegisterClassExW failed"));
		return nullptr;
	}

	HWND hwnd = CreateWindowExW(0,
								kRawInputWindowClass,
								L"",
								0,
								0,
								0,
								0,
								0,
								HWND_MESSAGE,
								nullptr,
								hInst,
								nullptr);
	if (hwnd == nullptr) {
		Locator::getLogger()->warn(
		  "{}",
		  werr_ssprintf(GetLastError(), "RawInput: CreateWindowExW failed"));
		return nullptr;
	}
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	RAWINPUTDEVICE rids[2]{};
	rids[0].usUsagePage = kHidUsagePageGeneric;
	rids[0].usUsage = kHidUsageGenericKeyboard;
	rids[0].dwFlags = RIDEV_INPUTSINK;
	rids[0].hwndTarget = hwnd;

	rids[1].usUsagePage = kHidUsagePageGeneric;
	rids[1].usUsage = kHidUsageGenericMouse;
	rids[1].dwFlags = RIDEV_INPUTSINK;
	rids[1].hwndTarget = hwnd;

	if (!RegisterRawInputDevices(rids, 2, sizeof(RAWINPUTDEVICE))) {
		Locator::getLogger()->warn(
		  "{}",
		  werr_ssprintf(GetLastError(),
						"RawInput: RegisterRawInputDevices failed"));
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		DestroyWindow(hwnd);
		return nullptr;
	}

	return hwnd;
}

void
InputHandler_Win32_RawInput::DestroySinkWindow(HWND hwnd)
{
	if (hwnd == nullptr)
		return;

	RAWINPUTDEVICE rids[2]{};
	rids[0].usUsagePage = kHidUsagePageGeneric;
	rids[0].usUsage = kHidUsageGenericKeyboard;
	rids[0].dwFlags = RIDEV_REMOVE;
	rids[0].hwndTarget = nullptr;

	rids[1].usUsagePage = kHidUsagePageGeneric;
	rids[1].usUsage = kHidUsageGenericMouse;
	rids[1].dwFlags = RIDEV_REMOVE;
	rids[1].hwndTarget = nullptr;

	RegisterRawInputDevices(rids, 2, sizeof(RAWINPUTDEVICE));
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
	DestroyWindow(hwnd);
}

InputHandler_Win32_RawInput::InputHandler_Win32_RawInput()
{
	if (PREFSMAN != nullptr && PREFSMAN->m_bThreadedInput) {
		m_hStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		m_hReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (m_hStopEvent != nullptr && m_hReadyEvent != nullptr) {
			m_InputThread.SetName("RawInput thread");
			m_InputThread.Create(InputThread_Start, this);
			WaitForSingleObject(m_hReadyEvent, INFINITE);
		}
		if (m_hReadyEvent != nullptr) {
			CloseHandle(m_hReadyEvent);
			m_hReadyEvent = nullptr;
		}
		if (!m_bInitialized) {
			if (m_InputThread.IsCreated())
				m_InputThread.Wait();
			if (m_hStopEvent != nullptr) {
				CloseHandle(m_hStopEvent);
				m_hStopEvent = nullptr;
			}
		}
	} else {
		m_hMsgWnd = CreateSinkWindow();
		m_bInitialized = (m_hMsgWnd != nullptr);
	}
	if (m_bInitialized) {
		g_bWin32RawInputActive.store(true, std::memory_order_release);
		Locator::getLogger()->info("Initialized Win32 RawInput ({}).",
								   m_InputThread.IsCreated() ? "threaded"
															 : "unthreaded");
	}
}

InputHandler_Win32_RawInput::~InputHandler_Win32_RawInput()
{
	if (m_InputThread.IsCreated()) {
		m_bShutdown.store(true, std::memory_order_release);
		if (m_hStopEvent != nullptr)
			SetEvent(m_hStopEvent);
		Locator::getLogger()->info("Shutting down RawInput thread ...");
		m_InputThread.Wait();
		Locator::getLogger()->info("RawInput thread shut down.");
	}

	if (m_hStopEvent != nullptr) {
		CloseHandle(m_hStopEvent);
		m_hStopEvent = nullptr;
	}
	if (m_hReadyEvent != nullptr) {
		CloseHandle(m_hReadyEvent);
		m_hReadyEvent = nullptr;
	}
	if (m_hMsgWnd != nullptr) {
		DestroySinkWindow(m_hMsgWnd);
		m_hMsgWnd = nullptr;
	}
	if (m_bInitialized) {
		g_bWin32RawInputActive.store(false, std::memory_order_release);
		m_bInitialized = false;
	}
}

int
InputHandler_Win32_RawInput::InputThread_Start(void* p)
{
	static_cast<InputHandler_Win32_RawInput*>(p)->InputThreadMain();
	return 0;
}

void
InputHandler_Win32_RawInput::InputThreadMain()
{
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
		Locator::getLogger()->warn(
		  "{}",
		  werr_ssprintf(GetLastError(),
						"Failed to set RawInput thread priority"));
	}
	SetThreadPriorityBoost(GetCurrentThread(), FALSE);

	HWND msgHwnd = CreateSinkWindow();
	m_bInitialized = (msgHwnd != nullptr);
	SetEvent(m_hReadyEvent);
	if (msgHwnd == nullptr)
		return;

	while (!m_bShutdown.load(std::memory_order_acquire)) {
		const DWORD waitRes =
		  MsgWaitForMultipleObjectsEx(1,
									  &m_hStopEvent,
									  INFINITE,
									  QS_RAWINPUT | QS_ALLPOSTMESSAGE,
									  MWMO_INPUTAVAILABLE);
		const auto now = std::chrono::steady_clock::now();
		if (waitRes == WAIT_OBJECT_0 ||
			m_bShutdown.load(std::memory_order_acquire)) {
			break;
		}
		if (waitRes == WAIT_FAILED) {
			Locator::getLogger()->trace(
			  "{}",
			  werr_ssprintf(GetLastError(),
							"RawInput: MsgWaitForMultipleObjectsEx failed"));
			break;
		}
		MSG msg;
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_INPUT) {
				HandleRawInput(reinterpret_cast<HRAWINPUT>(msg.lParam), now);
				DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
			} else if (msg.message == WM_QUIT) {
				m_bShutdown.store(true, std::memory_order_release);
				break;
			} else {
				DispatchMessageW(&msg);
			}
		}
	}

	DestroySinkWindow(msgHwnd);
}

void
InputHandler_Win32_RawInput::Update()
{
	if (!m_InputThread.IsCreated() && m_hMsgWnd != nullptr) {
		const auto now = std::chrono::steady_clock::now();
		MSG msg;
		while (PeekMessageW(&msg, m_hMsgWnd, WM_INPUT, WM_INPUT, PM_REMOVE)) {
			HandleRawInput(reinterpret_cast<HRAWINPUT>(msg.lParam), now);
			DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		}
		InputHandler::UpdateTimer();
	}
}

void
InputHandler_Win32_RawInput::WindowReset()
{
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		m_HeldKeys.reset();
		m_bFocused = false;
		m_iWheelRemainder = 0;
	}
	if (INPUTFILTER != nullptr) {
		INPUTFILTER->ResetDevice(DEVICE_KEYBOARD);
		INPUTFILTER->ResetDevice(DEVICE_MOUSE);
	}
}

void
InputHandler_Win32_RawInput::GetDevicesAndDescriptions(
  std::vector<InputDeviceInfo>& vDevicesOut)
{
	if (!m_bInitialized)
		return;

	vDevicesOut.emplace_back(DEVICE_KEYBOARD, "RawInput Keyboard");
	vDevicesOut.emplace_back(DEVICE_MOUSE, "RawInput Mouse");
}

void
InputHandler_Win32_RawInput::HandleRawInput(
  HRAWINPUT hRawInput,
  const std::chrono::steady_clock::time_point& now)
{
	alignas(RAWINPUT) BYTE stackBuf[sizeof(RAWINPUT) + 64];
	UINT cbSize = sizeof(stackBuf);
	UINT uWritten = GetRawInputData(
	  hRawInput, RID_INPUT, stackBuf, &cbSize, sizeof(RAWINPUTHEADER));
	const RAWINPUT* pRaw = reinterpret_cast<const RAWINPUT*>(stackBuf);
	if (uWritten == static_cast<UINT>(-1)) {
		cbSize = 0;
		if (GetRawInputData(
			  hRawInput, RID_INPUT, nullptr, &cbSize, sizeof(RAWINPUTHEADER)) !=
			  0 ||
			cbSize == 0) {
			return;
		}
		m_vRawBuffer.resize(cbSize);
		uWritten = GetRawInputData(hRawInput,
								   RID_INPUT,
								   m_vRawBuffer.data(),
								   &cbSize,
								   sizeof(RAWINPUTHEADER));
		if (uWritten == static_cast<UINT>(-1))
			return;
		pRaw = reinterpret_cast<const RAWINPUT*>(m_vRawBuffer.data());
	}

	if (INPUTFILTER == nullptr)
		return;

	const HWND hMain = GraphicsWindow::GetHwnd();
	if (hMain == nullptr || GetForegroundWindow() != hMain) {
		bool bNeedReset = false;
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			if (m_bFocused || m_HeldKeys.any()) {
				m_HeldKeys.reset();
				m_bFocused = false;
				m_iWheelRemainder = 0;
				bNeedReset = true;
			}
		}
		if (bNeedReset) {
			INPUTFILTER->ResetDevice(DEVICE_KEYBOARD);
			INPUTFILTER->ResetDevice(DEVICE_MOUSE);
		}
		return;
	}

	if (pRaw->header.dwType == RIM_TYPEKEYBOARD) {
		const RAWKEYBOARD& kb = pRaw->data.keyboard;
		if (kb.MakeCode == KEYBOARD_OVERRUN_MAKE_CODE)
			return;
		// Extended navigation and PrintScreen keys synthesize an E0-prefixed
		// Shift scancode; Pause (E1 1D 45) emits a trailing 0x45 packet with
		// VKey == 0xFF that would otherwise collide with DIK_NUMLOCK.
		if ((kb.Flags & RI_KEY_E0) != 0 &&
			(kb.MakeCode == 0x2A || kb.MakeCode == 0x36)) {
			return;
		}
		if (kb.MakeCode == 0x45 && kb.VKey == 0xFF)
			return;

		DeviceButton btn = DeviceButton_Invalid;
		if ((kb.Flags & RI_KEY_E1) != 0 || kb.VKey == VK_PAUSE ||
			(kb.MakeCode == 0x46 && (kb.Flags & RI_KEY_E0) != 0)) {
			btn = KEY_PAUSE;
		} else if (kb.VKey == VK_NUMLOCK) {
			btn = KEY_NUMLOCK;
		} else {
			const int sc = ((kb.Flags & RI_KEY_E0) != 0)
							 ? ((kb.MakeCode & 0x7F) | 0x80)
							 : (kb.MakeCode & 0x7F);
			const int mapped = ConvertWin32ScancodeToKey(sc);
			if (mapped == '?' || mapped < 0 || mapped >= NUM_DeviceButton)
				return;
			btn = static_cast<DeviceButton>(mapped);
		}

		const bool isDown = ((kb.Flags & RI_KEY_BREAK) == 0);
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			m_bFocused = true;
			if (isDown && m_HeldKeys.test(static_cast<size_t>(btn)))
				return;
			m_HeldKeys.set(static_cast<size_t>(btn), isDown);
		}
		ButtonPressed(
		  DeviceInput(DEVICE_KEYBOARD, btn, isDown ? 1.0f : 0.0f, now));
		return;
	}
	if (pRaw->header.dwType == RIM_TYPEMOUSE) {
		const RAWMOUSE& mouse = pRaw->data.mouse;
		const USHORT flags = mouse.usButtonFlags;
		int wheelDelta = 0;
		int wheelNotch = 0;
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			m_bFocused = true;
			if ((flags & RI_MOUSE_WHEEL) != 0) {
				wheelDelta =
				  static_cast<int>(static_cast<SHORT>(mouse.usButtonData));
				m_iWheelRemainder += wheelDelta;
				if (m_iWheelRemainder >= WHEEL_DELTA) {
					wheelNotch = 1;
					m_iWheelRemainder %= WHEEL_DELTA;
				} else if (m_iWheelRemainder <= -WHEEL_DELTA) {
					wheelNotch = -1;
					m_iWheelRemainder %= WHEEL_DELTA;
				}
			}
		}

		POINT pt{};
		if (GetCursorPos(&pt) && ScreenToClient(hMain, &pt)) {
			INPUTFILTER->UpdateCursorLocation(static_cast<float>(pt.x),
											  static_cast<float>(pt.y));
		}

		if ((flags & RI_MOUSE_LEFT_BUTTON_DOWN) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_LEFT, 1.0f, now));
		if ((flags & RI_MOUSE_LEFT_BUTTON_UP) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_LEFT, 0.0f, now));
		if ((flags & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_RIGHT, 1.0f, now));
		if ((flags & RI_MOUSE_RIGHT_BUTTON_UP) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_RIGHT, 0.0f, now));
		if ((flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_MIDDLE, 1.0f, now));
		if ((flags & RI_MOUSE_MIDDLE_BUTTON_UP) != 0)
			ButtonPressed(DeviceInput(DEVICE_MOUSE, MOUSE_MIDDLE, 0.0f, now));

		if ((flags & RI_MOUSE_WHEEL) != 0) {
			INPUTFILTER->UpdateMouseWheel(static_cast<float>(wheelDelta));
			if (wheelNotch > 0) {
				ButtonPressed(
				  DeviceInput(DEVICE_MOUSE, MOUSE_WHEELUP, 1.0f, now));
				ButtonPressed(
				  DeviceInput(DEVICE_MOUSE, MOUSE_WHEELUP, 0.0f, now));
			} else if (wheelNotch < 0) {
				ButtonPressed(
				  DeviceInput(DEVICE_MOUSE, MOUSE_WHEELDOWN, 1.0f, now));
				ButtonPressed(
				  DeviceInput(DEVICE_MOUSE, MOUSE_WHEELDOWN, 0.0f, now));
			}
		}
	}
}

wchar_t
InputHandler_Win32_RawInput::DeviceButtonToChar(DeviceButton button,
												bool bUseCurrentKeyModifiers)
{
	switch (button) {
		case KEY_ESC:
		case KEY_TAB:
		case KEY_ENTER:
		case KEY_BACK:
			return L'\0';
		default:
			break;
	}

	for (int sc = 0; sc < 256; ++sc) {
		if (ConvertWin32ScancodeToKey(sc) != button)
			continue;
		const HKL layout = GetKeyboardLayout(0);
		const UINT vk = MapVirtualKeyExW(static_cast<UINT>(sc), 1, layout);
		BYTE keys[256]{};
		if (bUseCurrentKeyModifiers)
			GetKeyboardState(keys);
		WCHAR result[2]{};
		if (ToUnicodeEx(
			  vk, static_cast<UINT>(sc), keys, result, 2, 0, layout) == 1 &&
			result[0] != L'\0') {
			return result[0];
		}
		break;
	}

	return InputHandler::DeviceButtonToChar(button, bUseCurrentKeyModifiers);
}
