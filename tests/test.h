/**
 * HUI tests - minimal test harness: no dependency, one executable per file.
 */

#ifndef HUI_TEST_H
#define HUI_TEST_H

#include <hui/hui.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_test_checks = 0;
static int g_test_failures = 0;
static int g_test_errors = 0;
static int g_test_expect_errors = 0;

#define CHECK(condition) do { \
	g_test_checks++; \
	if (!(condition)) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
	} \
} while (0)

#define CHECK_NEAR(actual, expected) do { \
	double test_a_ = (double)(actual), test_e_ = (double)(expected); \
	g_test_checks++; \
	if (fabs(test_a_ - test_e_) > 0.51) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #actual, test_a_, test_e_); \
	} \
} while (0)

#define CHECK_STR(actual, expected) do { \
	const char* test_a_ = (actual); \
	const char* test_e_ = (expected); \
	g_test_checks++; \
	if (!test_a_ || strcmp(test_a_, test_e_) != 0) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: %s = \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual, test_a_ ? test_a_ : "(null)", test_e_); \
	} \
} while (0)

/* Bounds of a widget in its window. */
#define CHECK_RECT(widget, ex, ey, ew, eh) do { \
	float test_x_, test_y_, test_w_, test_h_; \
	HUI_GetWidgetRect((widget), &test_x_, &test_y_, &test_w_, &test_h_); \
	g_test_checks++; \
	if (fabs(test_x_ - (ex)) > 0.51 || fabs(test_y_ - (ey)) > 0.51 || fabs(test_w_ - (ew)) > 0.51 || fabs(test_h_ - (eh)) > 0.51) { \
		g_test_failures++; \
		printf("  FAIL %s:%d: rect of %s = (%g, %g, %g, %g), expected (%g, %g, %g, %g)\n", __FILE__, __LINE__, #widget, \
			test_x_, test_y_, test_w_, test_h_, (double)(ex), (double)(ey), (double)(ew), (double)(eh)); \
	} \
} while (0)

/* Errors raised by the library fail the test, unless the test announced them. */
static inline void test_error_callback(HUI_EError code, HUI_ESeverity severity, const char* detail) {
	g_test_errors++;
	if (!g_test_expect_errors) {
		g_test_failures++;
		printf("  UNEXPECTED ERROR %s (%s): %s\n", HUI_ErrorEnumToString(code), HUI_SeverityEnumToString(severity), detail);
	}
}

#define EXPECT_ERRORS_BEGIN() do { g_test_expect_errors = 1; g_test_errors = 0; } while (0)
#define EXPECT_ERRORS_END(minimum) do { g_test_expect_errors = 0; CHECK(g_test_errors >= (minimum)); } while (0)

/* Starts the library without a window system and without a GPU. */
static inline int test_init(int worker_threads) {
	HUI_InitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = HUI_BACKEND_NULL;
	desc.headless = HUI_TRUE;
	desc.worker_threads = worker_threads;
	HUI_RegisterErrorCallback(test_error_callback);
	return HUI_InitEx(&desc);
}

#define RUN(function) do { printf("%s\n", #function); function(); } while (0)

static inline int test_report(const char* name) {
	printf("%s: %d checks, %d failures\n", name, g_test_checks, g_test_failures);
	return g_test_failures == 0 ? 0 : 1;
}

/* Moves the cursor to the middle of a widget and clicks. */
static inline void test_click(HUI_id window, HUI_id widget) {
	float x, y, w, h;
	HUI_GetWidgetRect(widget, &x, &y, &w, &h);
	HUI_InjectMouseMove(window, x + w * 0.5f, y + h * 0.5f);
	HUI_InjectMouseButton(window, HUI_MOUSE_LEFT, HUI_TRUE);
	HUI_InjectMouseButton(window, HUI_MOUSE_LEFT, HUI_FALSE);
}

static inline void test_click_at(HUI_id window, float x, float y) {
	HUI_InjectMouseMove(window, x, y);
	HUI_InjectMouseButton(window, HUI_MOUSE_LEFT, HUI_TRUE);
	HUI_InjectMouseButton(window, HUI_MOUSE_LEFT, HUI_FALSE);
}

static inline void test_key(HUI_id window, HUI_EKey key, HUI_uint modifiers) {
	HUI_InjectKey(window, key, HUI_TRUE, modifiers);
	HUI_InjectKey(window, key, HUI_FALSE, modifiers);
}

#endif /* HUI_TEST_H */
