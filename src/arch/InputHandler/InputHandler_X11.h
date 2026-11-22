/* InputHandler_X11 - X-based keyboard input handler. */

#ifndef INPUT_HANDLER_X11_H
#define INPUT_HANDLER_X11_H

#include "InputHandler.h"
#include "RageUtil/Misc/RageThreads.h"

#include <atomic>
#include <bitset>
#include <chrono>

struct _XDisplay;

class InputHandler_X11 : public InputHandler
{
  public:
	InputHandler_X11();
	~InputHandler_X11() override;
	void Update() override;
	void WindowReset() override;
	void GetDevicesAndDescriptions(
	  std::vector<InputDeviceInfo>& vDevicesOut) override;

  private:
	static int InputThread_Start(void* p);
	void InputThreadMain();
	void StartInputCollection();
	void StopInputCollection();
	void DrainPendingEvents(
	  _XDisplay* dpy,
	  unsigned long win,
	  const std::chrono::steady_clock::time_point& wakeTime);
	void RegisterKeyEvent(const std::chrono::steady_clock::time_point& timer,
						  bool keyDown,
						  InputDevice input,
						  DeviceButton button);

	RageThread m_InputThread;
	_XDisplay* m_pInputDpy{ nullptr };
	unsigned long m_BoundWin{ 0 };
	int m_WakePipe[2]{ -1, -1 };
	std::atomic<bool> m_bShutdown{ false };
	bool m_bUsingThread{ false };
	bool m_bDetectableAutoRepeat{ false };
	std::bitset<NUM_DeviceButton> m_HeldButtons{};
};

#endif
