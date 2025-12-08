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
 *  shm_mount_integration.h
 *  PHD Guiding
 *
 *  Integration wrapper for mount list shared memory
 *  Provides simpler C++ interface for PHD2 to manage mount SHM
 *
 */

#ifndef SHM_MOUNT_INTEGRATION_H_INCLUDED
#define SHM_MOUNT_INTEGRATION_H_INCLUDED

#include <wx/wx.h>

/**
 * C++ wrapper class for mount list shared memory management
 */
class MountSHMManager
{
public:
    /**
     * Initialize the mount SHM, creating it if necessary
     * @return true on success, false on error
     */
    static bool Initialize(void);

    /**
     * Shutdown and cleanup the mount SHM
     */
    static void Shutdown(void);

    /**
     * Update the mount list in shared memory
     * @param mounts Array of mount names
     * @return true on success, false on error
     */
    static bool UpdateMountList(const wxArrayString& mounts);
    
    /**
     * Get the mount list from shared memory
     * @param mounts Output array for mount names
     * @return Number of mounts read
     */
    static int GetMountList(wxArrayString& mounts);

    /**
     * Set the selected mount by index
     * @param index Index in the mount list (or -1 to deselect)
     * @return true on success, false on error
     */
    static bool SetSelectedMount(int index);

    /**
     * Get the selected mount index from shared memory
     * @return Mount index, or -1 if none selected
     */
    static int GetSelectedMount(void);

    /**
     * Check if a mount selection change has occurred
     * Useful for polling to detect external changes
     * @return true if selection has changed since last check
     */
    static bool HasSelectionChanged(void);

private:
    MountSHMManager() {}
    static unsigned int s_last_change_counter;
};

#endif // SHM_MOUNT_INTEGRATION_H_INCLUDED
