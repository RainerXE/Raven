/**
 * HUI - Horizon UI
 * Lightweight, multithreaded, retained-mode GUI library in C.
 *
 * hui_core.h : base types, lifecycle, main loop, threading, errors.
 *
 * Conventions (shared with the Horizon Rendering Library):
 *   - every object is an opaque HUI_id handle, HUI_INVALID_ID on failure
 *   - HUI_CreateX / HUI_DeleteX / HUI_IsValidX / HUI_SetX... / HUI_GetX...
 *   - enums are HUI_E..., callbacks are HUI_C...
 *   - booleans are ints (HUI_TRUE / HUI_FALSE), colors are float r, g, b, a
 *   - strings are UTF-8 and always copied by the library
 *
 * Threading: unless a function is documented as thread-safe, it must be
 * called from the UI thread (the thread that called HUI_Init).
 */

#ifndef HUI_CORE_H
#define HUI_CORE_H

#define HUI_VERSION_MAJOR 0
#define HUI_VERSION_MINOR 1
#define HUI_VERSION_PATCH 0
#define HUI_API_VERSION "0.1"

#ifdef __cplusplus
 #include <cstdint>
 #include <cstddef>
#else
 #include <stdint.h>
 #include <stddef.h>
#endif


/** shared library export / import */

#if defined(_WIN32)
	#if defined(HUI_BUILD_DLL)
		#define HUI_API __declspec(dllexport)
	#elif defined(HUI_USE_DLL)
		#define HUI_API __declspec(dllimport)
	#else
		#define HUI_API
	#endif
#elif defined(__GNUC__) && defined(HUI_BUILD_DLL)
	#define HUI_API __attribute__((visibility("default")))
#else
	#define HUI_API
#endif


/**
 * Opaque handle to any HUI object (window, widget, font, texture, timer,
 * tree item). A handle carries the object type and a generation counter, so
 * using a deleted handle is detected and reported instead of crashing.
 * Both HUI_INVALID_ID and 0 are never valid handles.
 */
typedef uint32_t HUI_id;
typedef uint32_t HUI_uint;

#define HUI_FALSE ((int)0)
#define HUI_TRUE  ((int)1)

#define HUI_INVALID_ID ((HUI_id)-1)


typedef enum HUI_EBackend{
	/** Built-in Vulkan renderer. */
	HUI_BACKEND_VULKAN = 0x0001,
	/** Renders nothing. Used for headless runs and unit tests. */
	HUI_BACKEND_NULL,
	/** Backend supplied with HUI_RegisterBackend (see hui_backend.h). */
	HUI_BACKEND_CUSTOM
}HUI_EBackend;

typedef enum HUI_EError{
	HUI_NO_ERROR = 0x0070,
	HUI_ERROR_INVALID_ID,
	HUI_ERROR_INVALID_ENUM,
	HUI_ERROR_INVALID_VALUE,
	HUI_ERROR_INVALID_OPERATION,
	HUI_ERROR_BACKEND,
	HUI_ERROR_PLATFORM,
	HUI_ERROR_OUT_OF_MEMORY,
	HUI_ERROR_INVALID_FILE_FORMAT,
	HUI_ERROR_FILE_NOT_FOUND,
	HUI_ERROR_XML,
	HUI_ERROR_WRONG_THREAD
}HUI_EError;

typedef enum HUI_ESeverity{
	HUI_SEVERITY_WEAK_WARNING = 0x0080,
	HUI_SEVERITY_WARNING,
	HUI_SEVERITY_ERROR,
	HUI_SEVERITY_FATAL
}HUI_ESeverity;


/**
 * Optional settings for HUI_InitEx. A zero-initialized structure selects the
 * default for every field, so new fields can be added without breaking code.
 */
typedef struct HUI_InitDesc {
	/** Rendering backend. 0 selects HUI_BACKEND_VULKAN. */
	HUI_EBackend backend;
	/** Application name reported to the graphics driver. May be NULL. */
	const char* app_name;
	/** Worker threads of the job system. 0 = one per spare CPU core, negative = none. */
	int worker_threads;
	/** HUI_TRUE renders on the UI thread instead of a dedicated render thread. */
	int disable_render_thread;
	/** HUI_TRUE presents without waiting for the vertical blank. */
	int disable_vsync;
	/** HUI_TRUE enables the backend debug layer (Vulkan validation) when installed. */
	int enable_validation;
	/** HUI_TRUE opens no OS window (null platform). Combine with HUI_BACKEND_NULL for tests. */
	int headless;
	/** Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. */
	float ui_scale;
} HUI_InitDesc;

/** Optional memory hooks. See HUI_SetAllocator. */
typedef struct HUI_Allocator {
	void* (*alloc)(size_t size, void* user_data);
	void* (*realloc)(void* ptr, size_t size, void* user_data);
	void  (*free)(void* ptr, void* user_data);
	void* user_data;
} HUI_Allocator;

/** Timings and counters of the last frame produced for a window. */
typedef struct HUI_FrameStats {
	double layout_ms;
	double paint_ms;
	double render_ms;
	HUI_uint widget_count;
	HUI_uint instance_count;
	HUI_uint draw_call_count;
	HUI_uint paint_job_count;
	uint64_t frame_index;
} HUI_FrameStats;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  INITIALIZATION & LIFECYCLE
	 * ============================================================================ */

	/**
	 * @brief Starts the library with default settings on the given backend.
	 * The calling thread becomes the UI thread.
	 * @return HUI_TRUE on success. On failure, see HUI_GetLastError.
	 */
	HUI_API int HUI_Init(HUI_EBackend backend);

	/**
	 * @brief Starts the library with explicit settings.
	 * @param desc Settings, or NULL for the defaults.
	 */
	HUI_API int HUI_InitEx(const HUI_InitDesc* desc);

	/**
	 * @brief Destroys every window, widget and resource, stops the worker and
	 * render threads and releases the backend.
	 */
	HUI_API void HUI_Shutdown(void);

	HUI_API int HUI_IsInitialized(void);

	/** @return The library version, e.g. "0.1.0". */
	HUI_API const char* HUI_GetVersion(void);

	/**
	 * @brief Replaces the memory functions. Must be called before HUI_Init.
	 * The functions are called from several threads and must be thread-safe.
	 */
	HUI_API void HUI_SetAllocator(const HUI_Allocator* allocator);


	/* ============================================================================
	 *  MAIN LOOP
	 * ============================================================================ */

	/**
	 * @brief Runs the event loop until HUI_Quit is called or the last window closes.
	 *
	 * The loop is event-driven: when nothing changes, the thread sleeps and no
	 * frame is produced. It is equivalent to:
	 *
	 *     while (HUI_IsRunning()) { HUI_WaitEvents(-1.0); HUI_Update(); }
	 *
	 * @return The exit code passed to HUI_Quit.
	 */
	HUI_API int HUI_Run(void);

	/** @brief Asks the event loop to stop. Thread-safe. */
	HUI_API void HUI_Quit(int exit_code);

	/** @return HUI_FALSE once HUI_Quit was called or every window is closed. */
	HUI_API int HUI_IsRunning(void);

	/** @brief Processes pending OS events without waiting. */
	HUI_API void HUI_PollEvents(void);

	/**
	 * @brief Sleeps until an OS event, a posted task, a timer or a pending redraw.
	 * @param timeout_seconds Maximum wait. Negative waits without limit.
	 */
	HUI_API void HUI_WaitEvents(double timeout_seconds);

	/**
	 * @brief Runs posted tasks and timers, then lays out, paints and submits
	 * every window that changed since the last call.
	 */
	HUI_API void HUI_Update(void);

	/**
	 * @brief Registers a function called once per HUI_Update, before layout.
	 * Use it to drive an application or game loop from HUI_Run.
	 */
	typedef void (*HUI_CFrame)(double delta_seconds, void* user_data);
	HUI_API void HUI_SetFrameCallback(HUI_CFrame callback, void* user_data);

	/** @return Seconds elapsed since HUI_Init, from a monotonic clock. Thread-safe. */
	HUI_API double HUI_GetTime(void);


	/* ============================================================================
	 *  THREADING
	 * ============================================================================ */

	typedef void (*HUI_CTask)(void* user_data);
	typedef void (*HUI_CParallelRange)(HUI_uint begin, HUI_uint end, void* user_data);

	/**
	 * @brief Queues a function to run on the UI thread during the next HUI_Update
	 * and wakes the event loop. Thread-safe: this is how worker threads touch the UI.
	 */
	HUI_API void HUI_PostTask(HUI_CTask task, void* user_data);

	/** @brief Runs a function on a worker thread of the job system. Thread-safe. */
	HUI_API void HUI_RunAsync(HUI_CTask task, void* user_data);

	/**
	 * @brief Splits [0, count) into ranges of at most `grain` items and runs them
	 * on the worker threads and the calling thread. Returns when all are done.
	 */
	HUI_API void HUI_ParallelFor(HUI_uint count, HUI_uint grain, HUI_CParallelRange function, void* user_data);

	/** @brief Wakes HUI_WaitEvents from any thread. */
	HUI_API void HUI_WakeUp(void);

	HUI_API int HUI_GetWorkerThreadCount(void);
	HUI_API int HUI_IsUIThread(void);


	/* ============================================================================
	 *  TIMERS
	 * ============================================================================ */

	typedef void (*HUI_CTimer)(HUI_id timer, void* user_data);

	/**
	 * @brief Calls a function on the UI thread after `interval_seconds`.
	 * @param repeat HUI_TRUE to fire repeatedly, HUI_FALSE to fire once and delete itself.
	 */
	HUI_API HUI_id HUI_CreateTimer(double interval_seconds, int repeat, HUI_CTimer callback, void* user_data);
	HUI_API void HUI_DeleteTimer(HUI_id timer);
	HUI_API int HUI_IsValidTimer(HUI_id timer);


	/* ============================================================================
	 *  ACTIONS
	 * ============================================================================ */

	/**
	 * Actions connect widgets to application code by name. A widget carries an
	 * action name (set from XML with OnClicked="file.save", or from C with
	 * HUI_SetWidgetAction); when its main event fires, the function bound to that
	 * name is called. Names are resolved when the event fires, so actions can be
	 * bound before or after the widgets are created.
	 */
	typedef void (*HUI_CAction)(HUI_id sender, const char* action, void* user_data);

	HUI_API void HUI_BindAction(const char* name, HUI_CAction callback, void* user_data);
	HUI_API void HUI_UnbindAction(const char* name);

	/** @brief Receives every action that has no bound function (e.g. a script bridge). */
	HUI_API void HUI_SetDefaultActionHandler(HUI_CAction callback, void* user_data);


	/* ============================================================================
	 *  CLIPBOARD
	 * ============================================================================ */

	HUI_API void HUI_SetClipboardText(const char* text);

	/** @return The clipboard text, or "" when empty. Valid until the next call. */
	HUI_API const char* HUI_GetClipboardText(void);


	/* ============================================================================
	 *  ERROR HANDLING
	 * ============================================================================ */

	/**
	 * @brief Retrieves and clears the last recorded error.
	 * @param detail   Optional output: human-readable description.
	 * @param severity Optional output: severity of the error.
	 * @return HUI_NO_ERROR when nothing was recorded.
	 */
	HUI_API HUI_EError HUI_GetLastError(const char** detail, HUI_ESeverity* severity);

	HUI_API const char* HUI_ErrorEnumToString(HUI_EError error);
	HUI_API const char* HUI_SeverityEnumToString(HUI_ESeverity severity);

	/**
	 * @brief Registers a function called whenever an error is raised.
	 * The function may be registered before HUI_Init. It is called on the
	 * thread that raised the error: errors of the renderer (and the messages
	 * of the Vulkan validation layers) come from the render thread.
	 */
	typedef void (*HUI_CErrorCallback)(HUI_EError code, HUI_ESeverity severity, const char* detail);
	HUI_API void HUI_RegisterErrorCallback(HUI_CErrorCallback callback);


	/* ============================================================================
	 *  DIAGNOSTICS
	 * ============================================================================ */

	HUI_API void HUI_GetFrameStats(HUI_id window, HUI_FrameStats* stats);

#ifdef __cplusplus
} /* extern "C" */
#endif


/**
 * @brief Prints the last HUI error to stdout, if any, with the source location.
 * Requires <stdio.h>.
 */
#define HUI_CheckErrors() do { \
	const char* hui_e_ = 0; \
	HUI_ESeverity hui_sev_ = HUI_SEVERITY_WEAK_WARNING; \
	HUI_EError hui_err_ = HUI_GetLastError(&hui_e_, &hui_sev_); \
	if (hui_err_ != HUI_NO_ERROR) \
		printf("[HUI] %s | %s | %s | %s:%d\n", HUI_ErrorEnumToString(hui_err_), \
			HUI_SeverityEnumToString(hui_sev_), hui_e_, __FILE__, __LINE__); \
	} while(0)

#endif /* HUI_CORE_H */
