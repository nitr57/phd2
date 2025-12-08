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
 *  shm_monitor.cpp
 *  PHD Guiding
 *
 *  Global shared memory monitor for headless and GUI modes
 *
 */

#include "phd.h"
#include "shm_monitor.h"
#include "shm_camera_integration.h"
#include "shm_mount_integration.h"

#include <pthread.h>
#include <unistd.h>

static pthread_t g_shm_monitor_thread = 0;
static volatile int g_shm_monitor_running = 0;

static void* shm_monitor_thread_func(void* arg)
{
    Debug.Write("SHM Monitor: Thread started\n");
    
    wxString last_camera_id;
    int last_camera_index = -2;
    int last_mount_index = -2;
    
    while (g_shm_monitor_running)
    {
        // Check for camera index changes
        int camera_index = CameraSHMManager::GetSelectedCamera();
        if (camera_index != last_camera_index)
        {
            last_camera_index = camera_index;
            if (camera_index >= 0 && pFrame)
            {
                // Camera selection changed via SHM - queue event to gear dialog if open
                if (pFrame->pGearDialog)
                {
                    wxThreadEvent evt(wxEVT_THREAD);
                    evt.SetInt(camera_index);
                    evt.SetString(wxT("camera"));
                    wxQueueEvent(pFrame->pGearDialog, evt.Clone());
                }
                Debug.Write(wxString::Format("SHM Monitor: Camera index changed to %d\n", camera_index));
            }
        }
        
        // Check for camera ID changes
        wxString camera_id = CameraSHMManager::GetSelectedCameraId();
        if (camera_id != last_camera_id)
        {
            last_camera_id = camera_id;
            if (!camera_id.IsEmpty() && pFrame)
            {
                Debug.Write(wxString::Format("SHM Monitor: Camera ID changed to %s\n", camera_id));
                
                // Queue event to gear dialog if open
                if (pFrame->pGearDialog)
                {
                    wxThreadEvent evt(wxEVT_THREAD);
                    evt.SetString(wxT("camera_id:") + camera_id);
                    wxQueueEvent(pFrame->pGearDialog, evt.Clone());
                }
            }
        }
        
        // Check for mount index changes
        int mount_index = MountSHMManager::GetSelectedMount();
        if (mount_index != last_mount_index)
        {
            last_mount_index = mount_index;
            if (mount_index >= 0)
            {
                Debug.Write(wxString::Format("SHM Monitor: Mount index changed to %d\n", mount_index));
                
                // Save mount selection to config for persistence (works in headless and GUI modes)
                wxArrayString mount_names;
                MountSHMManager::GetMountList(mount_names);
                if (mount_index >= 0 && mount_index < (int)mount_names.Count())
                {
                    pConfig->Profile.SetString("/scope/LastMenuChoice", mount_names[mount_index]);
                    Debug.Write(wxString::Format("SHM Monitor: Saved mount selection to config: %s\n", mount_names[mount_index]));
                }
            }
        }
        
        usleep(500000);  // Poll every 0.5 seconds
    }
    
    Debug.Write("SHM Monitor: Thread stopped\n");
    return NULL;
}

bool SHMMonitor::Start()
{
    if (g_shm_monitor_thread != 0)
    {
        return true;  // Already running
    }
    
    g_shm_monitor_running = 1;
    if (pthread_create(&g_shm_monitor_thread, NULL, shm_monitor_thread_func, NULL) != 0)
    {
        Debug.Write("SHM Monitor: Failed to create thread\n");
        g_shm_monitor_running = 0;
        return false;
    }
    
    Debug.Write("SHM Monitor: Started\n");
    return true;
}

void SHMMonitor::Stop()
{
    if (g_shm_monitor_thread == 0)
    {
        return;
    }
    
    g_shm_monitor_running = 0;
    pthread_join(g_shm_monitor_thread, NULL);
    g_shm_monitor_thread = 0;
    
    Debug.Write("SHM Monitor: Stopped\n");
}

bool SHMMonitor::IsRunning()
{
    return g_shm_monitor_running != 0;
}
