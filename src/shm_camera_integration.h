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
