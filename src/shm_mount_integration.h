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
