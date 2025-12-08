/*
 *  camera_config_manager.h
 *  PHD Guiding
 *
 *  Generic camera configuration manager for exposing camera options via SHM
 *
 */

#ifndef CAMERA_CONFIG_MANAGER_H
#define CAMERA_CONFIG_MANAGER_H

#include "shm_camera_config.h"

/**
 * Generic camera configuration manager
 * Each camera can publish its available options (like bitdepth)
 * and external clients can read/write them via SHM
 */
class CameraConfigManager
{
public:
    /**
     * Initialize the camera config shared memory
     */
    static void Initialize();

    /**
     * Publish a single option to shared memory
     * @param option_name Name of the option (e.g., "bitdepth")
     * @param current_value Current value of the option
     * @param min_value Minimum allowed value
     * @param max_value Maximum allowed value
     */
    static void PublishOption(const char* option_name, int current_value, int min_value = 0, int max_value = 255);

    /**
     * Check if a configuration option was updated via SHM and get the new value
     * @param option_name Name of the option to check
     * @param out_value Output parameter for the new value
     * @return true if the option was updated, false otherwise
     */
    static bool GetUpdatedOption(const char* option_name, int* out_value);

    /**
     * Clear all options (call before republishing a camera's config)
     */
    static void ClearOptions();

private:
    static uint32_t last_update_counter;
};

#endif // CAMERA_CONFIG_MANAGER_H
