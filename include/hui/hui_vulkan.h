/**
 * HUI - Horizon UI
 * hui_vulkan.h : Vulkan-specific API.
 *
 * IMPORTANT: this header intentionally does not include the Vulkan SDK.
 * Vulkan handles are exchanged as opaque pointers (dispatchable handles) or
 * 64-bit integers (non-dispatchable handles); cast them back in the source
 * file that includes <vulkan/vulkan.h>.
 *
 * HUI loads the Vulkan loader library at run time: no Vulkan library is
 * needed to link an application.
 *
 * Device selection: HUI uses a discrete GPU when there is one, then an
 * integrated one. The environment variable HUI_VK_DEVICE=<index> forces the
 * device with that index in the enumeration order of the driver.
 */

#ifndef HUI_VULKAN_H
#define HUI_VULKAN_H

#include "hui_core.h"

/** Memory exported by HUI_Vulkan_CreateSharedTexture. */
typedef struct HUI_SharedTextureInfo {
	/** POSIX: file descriptor (opaque fd). Windows: HANDLE (opaque win32). The caller owns it. */
	intptr_t handle;
	/** Size of the memory allocation backing the image, in bytes. */
	uint64_t allocation_size;
	int width;
	int height;
} HUI_SharedTextureInfo;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  SHARING A DEVICE (call before HUI_Init)
	 * ============================================================================ */

	/**
	 * By default HUI creates its own VkInstance and VkDevice. To make HUI render
	 * with the device of the application (or of HRL), create the instance and the
	 * device with the extensions returned below, then pass them here.
	 */
	HUI_API const char* const* HUI_Vulkan_GetRequiredInstanceExtensions(HUI_uint* count);
	HUI_API const char* const* HUI_Vulkan_GetRequiredDeviceExtensions(HUI_uint* count);

	/**
	 * @brief Makes HUI use an existing device. HUI does not destroy these objects.
	 * @param instance        VkInstance
	 * @param physical_device VkPhysicalDevice
	 * @param device          VkDevice
	 * @param queue           VkQueue supporting graphics and presentation
	 * @param queue_family    Family index of `queue`
	 */
	HUI_API void HUI_Vulkan_SetExternalDevice(void* instance, void* physical_device, void* device,
		void* queue, HUI_uint queue_family);

	/**
	 * @brief A VkQueue must not be used by two threads at once. When the queue
	 * is shared with the application, HUI calls `lock` before and `unlock`
	 * after each use of the queue from its render thread.
	 */
	typedef void (*HUI_CVulkanQueueLock)(void* user_data);
	HUI_API void HUI_Vulkan_SetQueueLockCallbacks(HUI_CVulkanQueueLock lock, HUI_CVulkanQueueLock unlock, void* user_data);


	/* ============================================================================
	 *  ACCESS TO THE DEVICE (after HUI_Init)
	 * ============================================================================ */

	/** Opaque VkInstance, or NULL before initialization / on another backend. */
	HUI_API void* HUI_Vulkan_GetInstance(void);
	/** Opaque VkPhysicalDevice. */
	HUI_API void* HUI_Vulkan_GetPhysicalDevice(void);
	/** Opaque VkDevice. */
	HUI_API void* HUI_Vulkan_GetDevice(void);
	/** Opaque VkQueue used for rendering and presentation. */
	HUI_API void* HUI_Vulkan_GetGraphicsQueue(void);
	HUI_API HUI_uint HUI_Vulkan_GetGraphicsQueueFamily(void);


	/* ============================================================================
	 *  TEXTURES
	 * ============================================================================ */

	/**
	 * @brief Wraps an application image so that widgets (Image, Viewport) can show it.
	 * The image must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL whenever HUI
	 * renders, and must outlive the texture. HUI does not destroy it: destroy it
	 * after HUI_DeleteTexture, which returns once HUI no longer uses the image.
	 *
	 * The values of the image are shown as they are (the UI is not color
	 * managed): use a UNORM view of an sRGB-encoded image. Alpha is honored, so
	 * clear the image with alpha 1 unless the viewport should be see-through.
	 *
	 * @param image_view Raw VkImageView handle (cast to uint64_t).
	 * @param sampler    Raw VkSampler handle, or 0 for the HUI linear sampler.
	 */
	HUI_API HUI_id HUI_Vulkan_CreateTextureFromImageView(uint64_t image_view, uint64_t sampler, int width, int height);

	/**
	 * @brief Points an existing wrapped texture to another image view (after a resize).
	 * Returns once HUI no longer uses the previous view, which can then be destroyed.
	 */
	HUI_API void HUI_Vulkan_SetTextureImageView(HUI_id texture, uint64_t image_view, uint64_t sampler, int width, int height);

	/**
	 * @brief Creates an RGBA8 texture whose memory can be imported by another
	 * API or process (OpenGL GL_EXT_memory_object, another Vulkan device).
	 * Requires VK_KHR_external_memory_fd (POSIX) or _win32 (Windows).
	 * @return HUI_INVALID_ID when the device cannot export memory.
	 */
	HUI_API HUI_id HUI_Vulkan_CreateSharedTexture(int width, int height);

	/** @brief Exports the memory of a shared texture. Each call returns a new handle. */
	HUI_API int HUI_Vulkan_GetSharedTextureInfo(HUI_id texture, HUI_SharedTextureInfo* info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUI_VULKAN_H */
