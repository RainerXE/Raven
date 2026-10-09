/**
 * HUI - Horizon UI
 * hui_window.h : OS windows, input enums and events.
 *
 * HUI creates and owns its windows (Win32, X11). Sizes and positions are in
 * logical units; multiply by HUI_GetWindowScale to get pixels.
 */

#ifndef HUI_WINDOW_H
#define HUI_WINDOW_H

#include "hui_core.h"


typedef enum HUI_EWindowFlags{
	HUI_WINDOW_DEFAULT    = 0,
	HUI_WINDOW_NO_RESIZE  = (1 << 0),
	HUI_WINDOW_HIDDEN     = (1 << 1),
	HUI_WINDOW_BORDERLESS = (1 << 2),
	HUI_WINDOW_MAXIMIZED  = (1 << 3)
}HUI_EWindowFlags;

typedef enum HUI_EWindowSystem{
	HUI_WINDOW_SYSTEM_NONE = 0x0090,
	HUI_WINDOW_SYSTEM_WIN32,
	HUI_WINDOW_SYSTEM_X11,
	HUI_WINDOW_SYSTEM_WAYLAND,
	HUI_WINDOW_SYSTEM_COCOA
}HUI_EWindowSystem;

/**
 * Native handles of a window or of a native viewport.
 *   Win32 : display = HINSTANCE, window = HWND
 *   X11   : display = Display*,  window = Window (XID), visual = VisualID
 */
typedef struct HUI_NativeWindow {
	HUI_EWindowSystem system;
	void* display;
	uintptr_t window;
	uintptr_t visual;
} HUI_NativeWindow;

typedef enum HUI_EMouseButton{
	HUI_MOUSE_LEFT = 0,
	HUI_MOUSE_RIGHT,
	HUI_MOUSE_MIDDLE,
	HUI_MOUSE_X1,
	HUI_MOUSE_X2
}HUI_EMouseButton;

typedef enum HUI_EModifier{
	HUI_MOD_SHIFT = (1 << 0),
	HUI_MOD_CTRL  = (1 << 1),
	HUI_MOD_ALT   = (1 << 2),
	HUI_MOD_SUPER = (1 << 3)
}HUI_EModifier;

typedef enum HUI_ECursor{
	HUI_CURSOR_ARROW = 0x00B0,
	HUI_CURSOR_IBEAM,
	HUI_CURSOR_HAND,
	HUI_CURSOR_CROSSHAIR,
	HUI_CURSOR_RESIZE_H,
	HUI_CURSOR_RESIZE_V,
	HUI_CURSOR_RESIZE_NWSE,
	HUI_CURSOR_RESIZE_NESW,
	HUI_CURSOR_MOVE,
	HUI_CURSOR_NOT_ALLOWED
}HUI_ECursor;

/** Keyboard keys. Printable keys use their upper-case ASCII code. */
typedef enum HUI_EKey{
	HUI_KEY_UNKNOWN = 0,
	HUI_KEY_SPACE = 32,
	HUI_KEY_APOSTROPHE = 39,
	HUI_KEY_COMMA = 44,
	HUI_KEY_MINUS = 45,
	HUI_KEY_PERIOD = 46,
	HUI_KEY_SLASH = 47,
	HUI_KEY_0 = 48, HUI_KEY_1, HUI_KEY_2, HUI_KEY_3, HUI_KEY_4,
	HUI_KEY_5, HUI_KEY_6, HUI_KEY_7, HUI_KEY_8, HUI_KEY_9,
	HUI_KEY_SEMICOLON = 59,
	HUI_KEY_EQUAL = 61,
	HUI_KEY_A = 65, HUI_KEY_B, HUI_KEY_C, HUI_KEY_D, HUI_KEY_E, HUI_KEY_F,
	HUI_KEY_G, HUI_KEY_H, HUI_KEY_I, HUI_KEY_J, HUI_KEY_K, HUI_KEY_L,
	HUI_KEY_M, HUI_KEY_N, HUI_KEY_O, HUI_KEY_P, HUI_KEY_Q, HUI_KEY_R,
	HUI_KEY_S, HUI_KEY_T, HUI_KEY_U, HUI_KEY_V, HUI_KEY_W, HUI_KEY_X,
	HUI_KEY_Y, HUI_KEY_Z,
	HUI_KEY_LEFT_BRACKET = 91,
	HUI_KEY_BACKSLASH = 92,
	HUI_KEY_RIGHT_BRACKET = 93,
	HUI_KEY_GRAVE = 96,

	HUI_KEY_ESCAPE = 256,
	HUI_KEY_ENTER,
	HUI_KEY_TAB,
	HUI_KEY_BACKSPACE,
	HUI_KEY_INSERT,
	HUI_KEY_DELETE,
	HUI_KEY_RIGHT,
	HUI_KEY_LEFT,
	HUI_KEY_DOWN,
	HUI_KEY_UP,
	HUI_KEY_PAGE_UP,
	HUI_KEY_PAGE_DOWN,
	HUI_KEY_HOME,
	HUI_KEY_END,
	HUI_KEY_CAPS_LOCK,
	HUI_KEY_PRINT_SCREEN,
	HUI_KEY_PAUSE,
	HUI_KEY_F1, HUI_KEY_F2, HUI_KEY_F3, HUI_KEY_F4, HUI_KEY_F5, HUI_KEY_F6,
	HUI_KEY_F7, HUI_KEY_F8, HUI_KEY_F9, HUI_KEY_F10, HUI_KEY_F11, HUI_KEY_F12,
	HUI_KEY_KP_0, HUI_KEY_KP_1, HUI_KEY_KP_2, HUI_KEY_KP_3, HUI_KEY_KP_4,
	HUI_KEY_KP_5, HUI_KEY_KP_6, HUI_KEY_KP_7, HUI_KEY_KP_8, HUI_KEY_KP_9,
	HUI_KEY_KP_DECIMAL,
	HUI_KEY_KP_DIVIDE,
	HUI_KEY_KP_MULTIPLY,
	HUI_KEY_KP_SUBTRACT,
	HUI_KEY_KP_ADD,
	HUI_KEY_KP_ENTER,
	HUI_KEY_LEFT_SHIFT,
	HUI_KEY_LEFT_CTRL,
	HUI_KEY_LEFT_ALT,
	HUI_KEY_LEFT_SUPER,
	HUI_KEY_RIGHT_SHIFT,
	HUI_KEY_RIGHT_CTRL,
	HUI_KEY_RIGHT_ALT,
	HUI_KEY_RIGHT_SUPER,
	HUI_KEY_MENU,
	HUI_KEY_COUNT
}HUI_EKey;

typedef enum HUI_EEventType{
	HUI_EVENT_NONE = 0x00C0,
	HUI_EVENT_MOUSE_MOVE,
	HUI_EVENT_MOUSE_DOWN,
	HUI_EVENT_MOUSE_UP,
	HUI_EVENT_MOUSE_WHEEL,
	HUI_EVENT_MOUSE_ENTER,
	HUI_EVENT_MOUSE_LEAVE,
	HUI_EVENT_KEY_DOWN,
	HUI_EVENT_KEY_UP,
	/** A character was typed. `codepoint` holds the Unicode scalar value. */
	HUI_EVENT_TEXT,
	HUI_EVENT_FOCUS_GAINED,
	HUI_EVENT_FOCUS_LOST
}HUI_EEventType;

/**
 * Input event delivered to a widget. Mouse and key events bubble from the
 * target widget to its parents until one of them handles the event.
 */
typedef struct HUI_Event {
	HUI_EEventType type;
	HUI_id window;
	/** Widget the event was first sent to. */
	HUI_id target;
	/** Cursor position in the window, in logical units. */
	float x, y;
	/** Cursor position relative to the widget receiving the event. */
	float local_x, local_y;
	/** Mouse movement since the previous event (HUI_EVENT_MOUSE_MOVE). */
	float delta_x, delta_y;
	/** Scroll amount in lines, positive = up / left (HUI_EVENT_MOUSE_WHEEL). */
	float wheel_x, wheel_y;
	HUI_EMouseButton button;
	/** 1 = single click, 2 = double click, ... (HUI_EVENT_MOUSE_DOWN). */
	int clicks;
	HUI_EKey key;
	/** Bitmask of HUI_EModifier. */
	HUI_uint modifiers;
	/** HUI_TRUE when the key event comes from auto-repeat. */
	int repeat;
	HUI_uint codepoint;
} HUI_Event;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  WINDOWS
	 * ============================================================================ */

	/**
	 * @brief Opens a top-level window.
	 * @param width,height Client size in logical units.
	 * @param flags        Bitmask of HUI_EWindowFlags.
	 */
	HUI_API HUI_id HUI_CreateWindow(const char* title, int width, int height, HUI_uint flags);

	/** @brief Closes a window and deletes its content widgets. */
	HUI_API void HUI_DeleteWindow(HUI_id window);
	HUI_API int HUI_IsValidWindow(HUI_id window);

	/**
	 * @brief Sets the root widget of the window. The window takes ownership of
	 * the widget; the previous content is deleted.
	 */
	HUI_API void HUI_SetWindowContent(HUI_id window, HUI_id widget);
	HUI_API HUI_id HUI_GetWindowContent(HUI_id window);

	HUI_API void HUI_SetWindowTitle(HUI_id window, const char* title);
	HUI_API void HUI_SetWindowSize(HUI_id window, int width, int height);
	HUI_API void HUI_GetWindowSize(HUI_id window, int* width, int* height);
	HUI_API void HUI_SetWindowMinSize(HUI_id window, int width, int height);
	HUI_API void HUI_SetWindowPosition(HUI_id window, int x, int y);
	HUI_API void HUI_GetWindowPosition(HUI_id window, int* x, int* y);
	/** @brief Size of the drawable area in pixels. */
	HUI_API void HUI_GetWindowFramebufferSize(HUI_id window, int* width, int* height);
	/** @return Pixels per logical unit (monitor scale x global UI scale). */
	HUI_API float HUI_GetWindowScale(HUI_id window);

	HUI_API void HUI_ShowWindow(HUI_id window);
	HUI_API void HUI_HideWindow(HUI_id window);
	HUI_API void HUI_MaximizeWindow(HUI_id window);
	HUI_API void HUI_MinimizeWindow(HUI_id window);
	HUI_API void HUI_RestoreWindow(HUI_id window);

	HUI_API void HUI_SetWindowClearColor(HUI_id window, float r, float g, float b, float a);

	/** @brief Asks for a new frame even if no widget changed. Thread-safe. */
	HUI_API void HUI_RequestWindowRedraw(HUI_id window);

	/** @brief HUI_TRUE redraws the window every frame (games, live viewports). */
	HUI_API void HUI_SetWindowContinuousRedraw(HUI_id window, int enabled);

	/**
	 * @brief Called when the user asks to close the window.
	 * Return HUI_TRUE to let the window close, HUI_FALSE to keep it open.
	 */
	typedef int (*HUI_CWindowClose)(HUI_id window, void* user_data);
	HUI_API void HUI_SetWindowCloseCallback(HUI_id window, HUI_CWindowClose callback, void* user_data);

	/** @brief Called after the window was resized. Sizes are in logical units. */
	typedef void (*HUI_CWindowResize)(HUI_id window, int width, int height, void* user_data);
	HUI_API void HUI_SetWindowResizeCallback(HUI_id window, HUI_CWindowResize callback, void* user_data);

	HUI_API void HUI_SetWindowUserHandle(HUI_id window, void* handle);
	HUI_API void* HUI_GetWindowUserHandle(HUI_id window);

	/** @brief Fills `native` with the OS handles of the window. */
	HUI_API int HUI_GetWindowNativeHandle(HUI_id window, HUI_NativeWindow* native);

	/**
	 * @brief Reads back the last rendered frame as tightly packed RGBA8 pixels.
	 * Blocks until the frame is rendered. Free the result with HUI_FreePixels.
	 * @return NULL when the backend cannot capture.
	 */
	HUI_API uint8_t* HUI_CaptureWindow(HUI_id window, int* width, int* height);
	HUI_API void HUI_FreePixels(uint8_t* pixels);


	/* ============================================================================
	 *  INPUT STATE & SYNTHETIC INPUT
	 * ============================================================================ */

	/** @brief Widget that has the keyboard focus in the window, or HUI_INVALID_ID. */
	HUI_API HUI_id HUI_GetFocusedWidget(HUI_id window);
	/** @brief Deepest widget under the cursor, or HUI_INVALID_ID. */
	HUI_API HUI_id HUI_GetHoveredWidget(HUI_id window);
	HUI_API void HUI_GetMousePosition(HUI_id window, float* x, float* y);

	/**
	 * Inject input as if it came from the OS. Positions are in logical units.
	 * Useful for automated tests, replays and when HUI is driven by a host
	 * application.
	 */
	HUI_API void HUI_InjectMouseMove(HUI_id window, float x, float y);
	HUI_API void HUI_InjectMouseButton(HUI_id window, HUI_EMouseButton button, int pressed);
	HUI_API void HUI_InjectMouseWheel(HUI_id window, float wheel_x, float wheel_y);
	HUI_API void HUI_InjectKey(HUI_id window, HUI_EKey key, int pressed, HUI_uint modifiers);
	HUI_API void HUI_InjectText(HUI_id window, const char* text);

	/**
	 * @brief Registers a keyboard shortcut active while the window has focus and
	 * no widget handled the key.
	 * @param action Name of the action to fire (see HUI_BindAction).
	 */
	HUI_API void HUI_RegisterShortcut(HUI_id window, HUI_EKey key, HUI_uint modifiers, const char* action);
	HUI_API void HUI_UnregisterShortcut(HUI_id window, HUI_EKey key, HUI_uint modifiers);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_WINDOW_H */
