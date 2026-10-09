/**
 * Raven
 * Lightweight, multithreaded, retained-mode GUI library in C.
 *
 * raven_core.h : base types, lifecycle, main loop, threading, errors.
 *
 * Conventions:
 *   - every object is an opaque Rid handle, R_INVALID_ID on failure
 *   - functions are R<object>_<action>: Rwindow_create, Rwindow_destroy,
 *     Rwindow_is_valid, Rwindow_set_title, Rwindow_get_size...
 *   - types are RName (RInitDesc, REvent, RKey), callbacks are RNameCallback,
 *     constants and enum values are R_NAME
 *   - booleans are ints (R_TRUE / R_FALSE), colors are float r, g, b, a
 *   - strings are UTF-8 and always copied by the library
 *
 * Threading: unless a function is documented as thread-safe, it must be
 * called from the UI thread (the thread that called Rinit).
 */

#ifndef RAVEN_CORE_H
#define RAVEN_CORE_H

#define RAVEN_VERSION_MAJOR 0
#define RAVEN_VERSION_MINOR 1
#define RAVEN_VERSION_PATCH 0
#define RAVEN_API_VERSION "0.1"

#ifdef __cplusplus
 #include <cstdint>
 #include <cstddef>
#else
 #include <stdint.h>
 #include <stddef.h>
#endif


/** shared library export / import */

#if defined(_WIN32)
	#if defined(RAVEN_BUILD_DLL)
		#define RAVEN_API __declspec(dllexport)
	#elif defined(RAVEN_USE_DLL)
		#define RAVEN_API __declspec(dllimport)
	#else
		#define RAVEN_API
	#endif
#elif defined(__GNUC__) && defined(RAVEN_BUILD_DLL)
	#define RAVEN_API __attribute__((visibility("default")))
#else
	#define RAVEN_API
#endif


/**
 * Opaque handle to any Raven object (window, widget, font, texture, timer,
 * tree item). A handle carries the object type and a generation counter, so
 * using a deleted handle is detected and reported instead of crashing.
 * Both R_INVALID_ID and 0 are never valid handles.
 */
typedef uint32_t Rid;
typedef uint32_t Ruint;

#define R_FALSE ((int)0)
#define R_TRUE  ((int)1)

#define R_INVALID_ID ((Rid)-1)


typedef enum RBackend{
	/** Built-in Vulkan renderer. */
	R_BACKEND_VULKAN = 0x0001,
	/** Renders nothing. Used for headless runs and unit tests. */
	R_BACKEND_NULL,
	/** Backend supplied with Rbackend_register (see raven_backend.h). */
	R_BACKEND_CUSTOM
}RBackend;

typedef enum RError{
	R_NO_ERROR = 0x0070,
	R_ERROR_INVALID_ID,
	R_ERROR_INVALID_ENUM,
	R_ERROR_INVALID_VALUE,
	R_ERROR_INVALID_OPERATION,
	R_ERROR_BACKEND,
	R_ERROR_PLATFORM,
	R_ERROR_OUT_OF_MEMORY,
	R_ERROR_INVALID_FILE_FORMAT,
	R_ERROR_FILE_NOT_FOUND,
	R_ERROR_XML,
	R_ERROR_WRONG_THREAD
}RError;

typedef enum RSeverity{
	R_SEVERITY_WEAK_WARNING = 0x0080,
	R_SEVERITY_WARNING,
	R_SEVERITY_ERROR,
	R_SEVERITY_FATAL
}RSeverity;


/**
 * Optional settings for Rinit_ex. A zero-initialized structure selects the
 * default for every field, so new fields can be added without breaking code.
 */
typedef struct RInitDesc {
	/** Rendering backend. 0 selects R_BACKEND_VULKAN. */
	RBackend backend;
	/** Application name reported to the graphics driver. May be NULL. */
	const char* app_name;
	/** Worker threads of the job system. 0 = one per spare CPU core, negative = none. */
	int worker_threads;
	/** R_TRUE renders on the UI thread instead of a dedicated render thread. */
	int disable_render_thread;
	/** R_TRUE presents without waiting for the vertical blank. */
	int disable_vsync;
	/** R_TRUE enables the backend debug layer (Vulkan validation) when installed. */
	int enable_validation;
	/** R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests. */
	int headless;
	/** Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0. */
	float ui_scale;
} RInitDesc;

/** Optional memory hooks. See Rset_allocator. */
typedef struct RAllocator {
	void* (*alloc)(size_t size, void* user_data);
	void* (*realloc)(void* ptr, size_t size, void* user_data);
	void  (*free)(void* ptr, void* user_data);
	void* user_data;
} RAllocator;

/** Timings and counters of the last frame produced for a window. */
typedef struct RFrameStats {
	double layout_ms;
	double paint_ms;
	double render_ms;
	Ruint widget_count;
	Ruint instance_count;
	Ruint draw_call_count;
	/** Glass: times the frame was copied and blurred to be seen through (0 without glass). */
	Ruint backdrop_count;
	Ruint paint_job_count;
	uint64_t frame_index;
} RFrameStats;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  INITIALIZATION & LIFECYCLE
	 * ============================================================================ */

	/**
	 * @brief Starts the library with default settings on the given backend.
	 * The calling thread becomes the UI thread.
	 * @return R_TRUE on success. On failure, see Rerror_get_last.
	 */
	RAVEN_API int Rinit(RBackend backend);

	/**
	 * @brief Starts the library with explicit settings.
	 * @param desc Settings, or NULL for the defaults.
	 */
	RAVEN_API int Rinit_ex(const RInitDesc* desc);

	/**
	 * @brief Destroys every window, widget and resource, stops the worker and
	 * render threads and releases the backend.
	 */
	RAVEN_API void Rshutdown(void);

	RAVEN_API int Ris_initialized(void);

	/** @return The library version, e.g. "0.1.0". */
	RAVEN_API const char* Rget_version(void);

	/**
	 * @brief Replaces the memory functions. Must be called before Rinit.
	 * The functions are called from several threads and must be thread-safe.
	 */
	RAVEN_API void Rset_allocator(const RAllocator* allocator);


	/* ============================================================================
	 *  MAIN LOOP
	 * ============================================================================ */

	/**
	 * @brief Runs the event loop until Rquit is called or the last window closes.
	 *
	 * The loop is event-driven: when nothing changes, the thread sleeps and no
	 * frame is produced. It is equivalent to:
	 *
	 *     while (Ris_running()) { Rwait_events(-1.0); Rupdate(); }
	 *
	 * @return The exit code passed to Rquit.
	 */
	RAVEN_API int Rrun(void);

	/** @brief Asks the event loop to stop. Thread-safe. */
	RAVEN_API void Rquit(int exit_code);

	/** @return R_FALSE once Rquit was called or every window is closed. */
	RAVEN_API int Ris_running(void);

	/** @brief Processes pending OS events without waiting. */
	RAVEN_API void Rpoll_events(void);

	/**
	 * @brief Sleeps until an OS event, a posted task, a timer or a pending redraw.
	 * @param timeout_seconds Maximum wait. Negative waits without limit.
	 */
	RAVEN_API void Rwait_events(double timeout_seconds);

	/**
	 * @brief Runs posted tasks and timers, then lays out, paints and submits
	 * every window that changed since the last call.
	 */
	RAVEN_API void Rupdate(void);

	/**
	 * @brief Registers a function called once per Rupdate, before layout.
	 * Use it to drive an application or game loop from Rrun.
	 */
	typedef void (*RFrameCallback)(double delta_seconds, void* user_data);
	RAVEN_API void Rset_frame_callback(RFrameCallback callback, void* user_data);

	/** @return Seconds elapsed since Rinit, from a monotonic clock. Thread-safe. */
	RAVEN_API double Rget_time(void);


	/* ============================================================================
	 *  THREADING
	 * ============================================================================ */

	typedef void (*RTaskCallback)(void* user_data);
	typedef void (*RParallelRangeCallback)(Ruint begin, Ruint end, void* user_data);

	/**
	 * @brief Queues a function to run on the UI thread during the next Rupdate
	 * and wakes the event loop. Thread-safe: this is how worker threads touch the UI.
	 */
	RAVEN_API void Rtask_post(RTaskCallback task, void* user_data);

	/** @brief Runs a function on a worker thread of the job system. Thread-safe. */
	RAVEN_API void Rtask_run_async(RTaskCallback task, void* user_data);

	/**
	 * @brief Splits [0, count) into ranges of at most `grain` items and runs them
	 * on the worker threads and the calling thread. Returns when all are done.
	 */
	RAVEN_API void Rparallel_for(Ruint count, Ruint grain, RParallelRangeCallback function, void* user_data);

	/** @brief Wakes Rwait_events from any thread. */
	RAVEN_API void Rwake_up(void);

	RAVEN_API int Rget_worker_thread_count(void);
	RAVEN_API int Ris_ui_thread(void);


	/* ============================================================================
	 *  TIMERS
	 * ============================================================================ */

	typedef void (*RTimerCallback)(Rid timer, void* user_data);

	/**
	 * @brief Calls a function on the UI thread after `interval_seconds`.
	 * @param repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
	 */
	RAVEN_API Rid Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, void* user_data);
	RAVEN_API void Rtimer_destroy(Rid timer);
	RAVEN_API int Rtimer_is_valid(Rid timer);


	/* ============================================================================
	 *  ACTIONS
	 * ============================================================================ */

	/**
	 * Actions connect widgets to application code by name. A widget carries an
	 * action name (set from XML with OnClicked="file.save", or from C with
	 * Rwidget_set_action); when its main event fires, the function bound to that
	 * name is called. Names are resolved when the event fires, so actions can be
	 * bound before or after the widgets are created.
	 */
	typedef void (*RActionCallback)(Rid sender, const char* action, void* user_data);

	RAVEN_API void Raction_bind(const char* name, RActionCallback callback, void* user_data);
	RAVEN_API void Raction_unbind(const char* name);

	/** @brief Receives every action that has no bound function (e.g. a script bridge). */
	RAVEN_API void Raction_set_default_handler(RActionCallback callback, void* user_data);


	/* ============================================================================
	 *  CLIPBOARD
	 * ============================================================================ */

	RAVEN_API void Rclipboard_set_text(const char* text);

	/** @return The clipboard text, or "" when empty. Valid until the next call. */
	RAVEN_API const char* Rclipboard_get_text(void);


	/* ============================================================================
	 *  ERROR HANDLING
	 * ============================================================================ */

	/**
	 * @brief Retrieves and clears the last recorded error.
	 * @param detail   Optional output: human-readable description.
	 * @param severity Optional output: severity of the error.
	 * @return R_NO_ERROR when nothing was recorded.
	 */
	RAVEN_API RError Rerror_get_last(const char** detail, RSeverity* severity);

	RAVEN_API const char* Rerror_to_string(RError error);
	RAVEN_API const char* Rseverity_to_string(RSeverity severity);

	/**
	 * @brief Registers a function called whenever an error is raised.
	 * The function may be registered before Rinit. It is called on the
	 * thread that raised the error: errors of the renderer (and the messages
	 * of the Vulkan validation layers) come from the render thread.
	 */
	typedef void (*RErrorCallback)(RError code, RSeverity severity, const char* detail);
	RAVEN_API void Rerror_set_callback(RErrorCallback callback);


	/* ============================================================================
	 *  DIAGNOSTICS
	 * ============================================================================ */

	RAVEN_API void Rwindow_get_frame_stats(Rid window, RFrameStats* stats);

#ifdef __cplusplus
} /* extern "C" */
#endif


/**
 * @brief Prints the last Raven error to stdout, if any, with the source location.
 * Requires <stdio.h>.
 */
#define Rerror_check() do { \
	const char* rvn_e_ = 0; \
	RSeverity rvn_sev_ = R_SEVERITY_WEAK_WARNING; \
	RError rvn_err_ = Rerror_get_last(&rvn_e_, &rvn_sev_); \
	if (rvn_err_ != R_NO_ERROR) \
		printf("[Raven] %s | %s | %s | %s:%d\n", Rerror_to_string(rvn_err_), \
			Rseverity_to_string(rvn_sev_), rvn_e_, __FILE__, __LINE__); \
	} while(0)

#endif /* RAVEN_CORE_H */
