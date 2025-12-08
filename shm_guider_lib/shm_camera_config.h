/*
 *  shm_camera_config.h
 *  PHD Guiding
 *
 *  POSIX Shared Memory for camera configuration
 *
 */

#ifndef SHM_CAMERA_CONFIG_H
#define SHM_CAMERA_CONFIG_H

#include <stdint.h>

#define SHM_CAMERA_CONFIG_MAX_OPTIONS 8
#define SHM_CAMERA_CONFIG_OPTION_NAME_LEN 32
#define SHM_CAMERA_CONFIG_MAGIC 0x4341
#define SHM_CAMERA_CONFIG_SEM_NAME "/phd2_camera_config_sem"

/**
 * A single camera configuration option
 */
typedef struct {
    char name[SHM_CAMERA_CONFIG_OPTION_NAME_LEN];  // Option name (e.g., "bitdepth")
    int value;                                      // Current value
    int min_value;                                  // Min value
    int max_value;                                  // Max value
    uint8_t reserved[20];                           // Reserved for future use
} CameraConfigOption;

/**
 * Main shared memory structure for camera configuration
 */
typedef struct {
    uint32_t magic;                              // Magic number
    uint32_t version;                            // Version of this structure
    uint32_t num_options;                        // Number of available options
    uint32_t update_counter;                     // Counter incremented when options change
    uint8_t reserved[40];                        // Reserved for future expansion
    CameraConfigOption options[SHM_CAMERA_CONFIG_MAX_OPTIONS];
} CameraConfigSHM;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize camera config shared memory
 * @param create If true, create the SHM if it doesn't exist
 * @return Pointer to SHM structure, or NULL on error
 */
CameraConfigSHM* shm_camera_config_init(int create);

/**
 * Clean up shared memory
 * @param shm Pointer to SHM structure
 * @param unlink If true, unlink the SHM so it's deleted when all processes close it
 */
void shm_camera_config_cleanup(CameraConfigSHM* shm, int unlink);

/**
 * Get read-only access to shared memory
 * @return Pointer to SHM structure, or NULL on error
 */
const CameraConfigSHM* shm_camera_config_get_readonly(void);

/**
 * Release read-only access (no-op, kept for compatibility)
 */
void shm_camera_config_release_readonly(const CameraConfigSHM* shm);

/**
 * Set or update a configuration option value
 * @param shm Pointer to SHM structure
 * @param option_name Name of option
 * @param value New value
 * @return 0 on success, -1 on error
 */
int shm_camera_config_set_option(CameraConfigSHM* shm, const char* option_name, int value);

/**
 * Get a configuration option value from shared memory
 * @param option_name Name of option
 * @param value Output pointer for value
 * @return 0 on success, -1 on error or not found
 */
int shm_camera_config_get_option(const char* option_name, int* value);

#ifdef __cplusplus
}
#endif

#endif // SHM_CAMERA_CONFIG_H
