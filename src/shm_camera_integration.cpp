/*
 *  shm_camera_integration.cpp
 *  PHD Guiding
 *
 *  Integration wrapper for camera list shared memory
 *
 */

#include "phd.h"
#include "shm_camera_integration.h"
#include "shm_camera.h"

// Static member initialization
unsigned int CameraSHMManager::s_last_change_counter = 0;

// Global pointer to shared memory
static CameraListSHM* g_camera_shm = NULL;

bool CameraSHMManager::Initialize(void)
{
    if (g_camera_shm != NULL)
    {
        return true; // Already initialized
    }

    g_camera_shm = shm_camera_init(1); // Create if missing
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: Failed to initialize shared memory\n");
        return false;
    }

    Debug.Write("CameraSHMManager: Shared memory initialized\n");
    return true;
}

void CameraSHMManager::Shutdown(void)
{
    if (g_camera_shm != NULL)
    {
        shm_camera_cleanup(g_camera_shm, 1); // Unlink on cleanup
        g_camera_shm = NULL;
        Debug.Write("CameraSHMManager: Shared memory shut down\n");
    }
}

bool CameraSHMManager::UpdateCameraList(const wxArrayString& cameras)
{
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: SHM not initialized\n");
        return false;
    }

    // Convert wxArrayString to C-style array
    const char** camera_names = new const char*[cameras.Count()];
    for (size_t i = 0; i < cameras.Count(); i++)
    {
        camera_names[i] = cameras[i].c_str();
    }

    int result = shm_camera_update_list(g_camera_shm, camera_names, cameras.Count());

    delete[] camera_names;

    if (result != 0)
    {
        Debug.Write("CameraSHMManager: Failed to update camera list\n");
        return false;
    }

    // Signal all waiting clients that the list has changed
    shm_camera_signal_list_changed();

    Debug.Write(wxString::Format("CameraSHMManager: Updated camera list with %zu cameras\n", cameras.Count()));
    return true;
}

bool CameraSHMManager::SetSelectedCamera(int index)
{
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: SHM not initialized\n");
        return false;
    }

    uint32_t shm_index = (index < 0) ? INVALID_CAMERA_INDEX : (uint32_t)index;

    int result = shm_camera_set_selected(g_camera_shm, shm_index);

    if (result != 0)
    {
        Debug.Write(wxString::Format("CameraSHMManager: Failed to set selected camera: %d\n", index));
        return false;
    }

    // Signal all waiting clients that the selection has changed
    shm_camera_signal_selected_changed();

    Debug.Write(wxString::Format("CameraSHMManager: Selected camera index: %d\n", index));
    return true;
}

int CameraSHMManager::GetSelectedCamera(void)
{
    if (g_camera_shm == NULL)
    {
        return -1;
    }

    uint32_t index = shm_camera_get_selected(g_camera_shm);
    if (index == INVALID_CAMERA_INDEX)
    {
        return -1;
    }

    return (int)index;
}

bool CameraSHMManager::HasSelectionChanged(void)
{
    if (g_camera_shm == NULL)
    {
        return false;
    }

    uint32_t current_counter = g_camera_shm->selected_change_counter;
    if (current_counter != s_last_change_counter)
    {
        s_last_change_counter = current_counter;
        return true;
    }

    return false;
}
