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
 *  shm_camera_integration.h
 *  PHD Guiding
 *
 *  Integration wrapper for camera list shared memory
 *  Provides simpler C++ interface for PHD2 to manage camera SHM
 *
 */

#ifndef SHM_CAMERA_INTEGRATION_H_INCLUDED
#define SHM_CAMERA_INTEGRATION_H_INCLUDED

#include <wx/wx.h>

/**
 * C++ wrapper class for camera list shared memory management
 */
class CameraSHMManager
{
public:
    /**
     * Initialize the camera SHM, creating it if necessary
     * @return true on success, false on error
     */
    static bool Initialize(void);

    /**
     * Shutdown and cleanup the camera SHM
     */
    static void Shutdown(void);

    /**
     * Update the camera list in shared memory
     * @param cameras Array of camera names
     * @return true on success, false on error
     */
    static bool UpdateCameraList(const wxArrayString& cameras);

    /**
     * Set the selected camera by index
     * @param index Index in the camera list (or -1 to deselect)
     * @return true on success, false on error
     */
    static bool SetSelectedCamera(int index);

    /**
     * Get the selected camera index from shared memory
     * @return Camera index, or -1 if none selected
     */
    static int GetSelectedCamera(void);

    /**
     * Set the selected camera ID/instance
     * @param camera_id ID or instance identifier of the camera
     * @return true on success, false on error
     */
    static bool SetSelectedCameraId(const wxString& camera_id);

    /**
     * Get the selected camera ID/instance from shared memory
     * @return Camera ID string
     */
    static wxString GetSelectedCameraId(void);

    /**
     * Check if camera instance selection is available
     * @return true if the current camera supports instance selection
     */
    static bool CanSelectCamera(void);
    
    /**
     * Set the camera selection capability flag
     * Call this when a camera is selected to indicate if it supports instance selection
     * @param can_select true if camera supports selection, false otherwise
     * @return true on success
     */
    static bool SetCanSelectCamera(bool can_select);

    /**
     * Update available camera instances in shared memory
     * @param instances Array of available camera instances
     * @return true on success, false on error
     */
    static bool UpdateCameraInstances(const wxArrayString& display_names, const wxArrayString& ids);

    /**
     * Get available camera instances from shared memory
     * @param display_names Output array for display names
     * @param ids Output array for instance IDs
     * @return Number of instances read
     */
    static int GetCameraInstances(wxArrayString& display_names, wxArrayString& ids);

    /**
     * Check if a camera selection change has occurred
     * Useful for polling to detect external changes
     * @return true if selection has changed since last check
     */
    static bool HasSelectionChanged(void);

private:
    CameraSHMManager() {}
    static unsigned int s_last_change_counter;
};

#endif // SHM_CAMERA_INTEGRATION_H_INCLUDED
