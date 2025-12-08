/*
 *  Created by Nico Trost.
 *  Copyright (c) 2025-2025 Nico Trost.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Nico Trost nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

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

bool CameraSHMManager::SetSelectedCameraId(const wxString& camera_id)
{
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: SHM not initialized\n");
        return false;
    }

    std::string id(camera_id.c_str());
    int result = shm_camera_write_selected_id(id.c_str());

    if (result != 0)
    {
        Debug.Write(wxString::Format("CameraSHMManager: Failed to set selected camera ID: %s\n", camera_id));
        return false;
    }

    Debug.Write(wxString::Format("CameraSHMManager: Selected camera ID: %s\n", camera_id));
    return true;
}

wxString CameraSHMManager::GetSelectedCameraId(void)
{
    if (g_camera_shm == NULL)
    {
        return wxEmptyString;
    }

    return wxString(g_camera_shm->selected_camera_id);
}

bool CameraSHMManager::CanSelectCamera(void)
{
    if (g_camera_shm == NULL)
    {
        return false;
    }

    return g_camera_shm->can_select_camera != 0;
}

bool CameraSHMManager::SetCanSelectCamera(bool can_select)
{
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: SHM not initialized\n");
        return false;
    }

    g_camera_shm->can_select_camera = can_select ? 1 : 0;
    Debug.Write(wxString::Format("CameraSHMManager: Set can_select_camera = %d\n", g_camera_shm->can_select_camera));
    return true;
}

bool CameraSHMManager::UpdateCameraInstances(const wxArrayString& display_names, const wxArrayString& ids)
{
    if (g_camera_shm == NULL)
    {
        Debug.Write("CameraSHMManager: SHM not initialized\n");
        return false;
    }

    size_t num_instances = display_names.Count();
    if (num_instances > 64)  // MAX_CAMERA_INSTANCES
        num_instances = 64;

    CameraInstance instances[num_instances];
    for (size_t i = 0; i < num_instances; i++)
    {
        strncpy(instances[i].display_name, display_names[i].c_str(), MAX_CAMERA_NAME_LEN - 1);
        instances[i].display_name[MAX_CAMERA_NAME_LEN - 1] = '\0';

        strncpy(instances[i].id, ids[i].c_str(), MAX_CAMERA_NAME_LEN - 1);
        instances[i].id[MAX_CAMERA_NAME_LEN - 1] = '\0';
    }

    int result = shm_camera_update_instances(g_camera_shm, instances, (uint32_t)num_instances);

    if (result != 0)
    {
        Debug.Write("CameraSHMManager: Failed to update camera instances\n");
        return false;
    }

    Debug.Write(wxString::Format("CameraSHMManager: Updated camera instances with %zu instances (can_select=%u)\n", num_instances, g_camera_shm->can_select_camera));
    return true;
}

int CameraSHMManager::GetCameraInstances(wxArrayString& display_names, wxArrayString& ids)
{
    if (g_camera_shm == NULL)
    {
        return 0;
    }

    display_names.Clear();
    ids.Clear();

    for (uint32_t i = 0; i < g_camera_shm->num_instances; i++)
    {
        display_names.Add(wxString(g_camera_shm->instances[i].display_name));
        ids.Add(wxString(g_camera_shm->instances[i].id));
    }

    return (int)g_camera_shm->num_instances;
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
