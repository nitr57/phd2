/*
 *  camera_config_manager.cpp
 *  PHD Guiding
 *
 *  Generic camera configuration manager for exposing camera options via SHM
 *
 */

#include "phd.h"
#include "camera_config_manager.h"
#include <cstring>

uint32_t CameraConfigManager::last_update_counter = 0;

void CameraConfigManager::Initialize()
{
    shm_camera_config_init(1);
}

void CameraConfigManager::PublishOption(const char* option_name, int current_value, int min_value, int max_value)
{
    CameraConfigSHM* shm = shm_camera_config_init(0);
    if (!shm) {
        return;
    }

    // Find or create the option
    int idx = -1;
    for (uint32_t i = 0; i < shm->num_options; i++) {
        if (strcmp(shm->options[i].name, option_name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        if (shm->num_options >= SHM_CAMERA_CONFIG_MAX_OPTIONS) {
            return;  // No space for new option
        }
        idx = shm->num_options;
        shm->num_options++;
        memset(&shm->options[idx], 0, sizeof(CameraConfigOption));
        strncpy(shm->options[idx].name, option_name, SHM_CAMERA_CONFIG_OPTION_NAME_LEN - 1);
    }

    shm->options[idx].value = current_value;
    shm->options[idx].min_value = min_value;
    shm->options[idx].max_value = max_value;
    // Don't increment counter here - counter is only incremented when client modifies
}

bool CameraConfigManager::GetUpdatedOption(const char* option_name, int* out_value)
{
    const CameraConfigSHM* shm = shm_camera_config_get_readonly();
    if (!shm || !option_name || !out_value) {
        return false;
    }

    // Check if counter has changed
    if (shm->update_counter <= last_update_counter) {
        return false;
    }

    // Find the option
    for (uint32_t i = 0; i < shm->num_options; i++) {
        if (strcmp(shm->options[i].name, option_name) == 0) {
            *out_value = shm->options[i].value;
            last_update_counter = shm->update_counter;
            return true;
        }
    }

    return false;
}

void CameraConfigManager::ClearOptions()
{
    CameraConfigSHM* shm = shm_camera_config_init(0);
    if (!shm) {
        return;
    }

    // Clear all option structures to prevent garbage data when switching cameras
    shm->num_options = 0;
    for (uint32_t i = 0; i < SHM_CAMERA_CONFIG_MAX_OPTIONS; i++) {
        memset(&shm->options[i], 0, sizeof(CameraConfigOption));
    }
    // Increment counter so clients know the options have changed
    shm->update_counter++;
}
