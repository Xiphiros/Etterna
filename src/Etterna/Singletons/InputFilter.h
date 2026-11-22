/* InputFilter - Checks RageInput and generates a list of InputEvents,
 * representing button presses, releases, and repeats. */

#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H

#include "RageUtil/Misc/RageInputDevice.h"

#include <atomic>

enum InputEventType
{
	// The device was just pressed.
	IET_FIRST_PRESS,

	/* The device is auto-repeating. This event is guaranteed to be sent only
	 * between IET_FIRST_PRESS and IET_RELEASE pairs. */
	IET_REPEAT,

	/* The device is no longer pressed. Exactly one IET_RELEASE event will be
	 * sent for each IET_FIRST_PRESS. */
	IET_RELEASE,

	NUM_InputEventType,
	InputEventType_Invalid
};

const std::string&
InputEventTypeToString(InputEventType cat);
const std::string&
InputEventTypeToLocalizedString(InputEventType cat);
LuaDeclareType(InputEventType);

struct InputEvent
{
	InputEvent() = default;

	DeviceInput di;
	InputEventType type{ IET_FIRST_PRESS };

	// A list of all buttons that were pressed at the time of this event:
	DeviceInputList m_ButtonState;
};

struct MouseCoordinates
{
	float fX;
	float fY;
	float fZ;
};

class RageMutex;
struct ButtonState;
class InputFilter
{
  public:
	void ButtonPressed(const DeviceInput& di);
	void SetButtonComment(const DeviceInput& di,
						  const std::string& sComment = "");
	void ResetDevice(InputDevice dev);

	InputFilter();
	~InputFilter();
	void Reset();
	void Update(float fDeltaTime);

	void SetRepeatRate(float fRepeatRate);
	void SetRepeatDelay(float fDelay);
	void ResetRepeatRate();
	void ResetKeyRepeat(const DeviceInput& di);
	void RepeatStopKey(const DeviceInput& di);

	// If aButtonState is NULL, use the last reported state.
	bool IsBeingPressed(const DeviceInput& di,
						const DeviceInputList* pButtonState = nullptr) const;
	bool IsKBKeyPressed(DeviceButton k) const;
	bool IsControlPressed() const;
	bool IsShiftPressed() const;
	float GetSecsHeld(const DeviceInput& di,
					  const DeviceInputList* pButtonState = nullptr) const;
	float GetLevel(const DeviceInput& di,
				   const DeviceInputList* pButtonState = nullptr) const;
	std::string GetButtonComment(const DeviceInput& di) const;

	void GetInputEvents(std::vector<InputEvent>& aEventOut);
	void GetPressedButtons(std::vector<DeviceInput>& array) const;

	// cursor
	void UpdateCursorLocation(float _fX, float _fY);
	void UpdateMouseWheel(float _fZ);
	float GetCursorX() const
	{
		return m_fCursorX.load(std::memory_order_relaxed);
	}
	float GetCursorY() const
	{
		return m_fCursorY.load(std::memory_order_relaxed);
	}
	float GetMouseWheel() const
	{
		return m_fMouseWheel.load(std::memory_order_relaxed);
	}

	// Lua
	void PushSelf(lua_State* L);

	bool IsCapsLockEnabled() const
	{
		return capsLockEnabled.load(std::memory_order_relaxed);
	}

  private:
	void CheckButtonChange(ButtonState& bs,
						   DeviceInput di,
						   const std::chrono::steady_clock::time_point& now);
	void ReportButtonChange(const DeviceInput& di, InputEventType t);
	void MakeButtonStateList(std::vector<DeviceInput>& aInputOut) const;

	std::vector<InputEvent> queue;
	RageMutex* queuemutex;
	std::atomic<float> m_fCursorX{ 0.0f };
	std::atomic<float> m_fCursorY{ 0.0f };
	std::atomic<float> m_fMouseWheel{ 0.0f };

	InputFilter(const InputFilter& rhs);
	InputFilter& operator=(const InputFilter& rhs);

	std::atomic<bool> capsLockEnabled{ false };
};

extern InputFilter*
  INPUTFILTER; // global and accessible from anywhere in our program

#endif
