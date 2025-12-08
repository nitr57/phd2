/*
 *  camera_config_monitor.h
 *  PHD Guiding
 *
 *  Monitor thread for camera configuration changes via SHM
 *
 */

#ifndef CAMERA_CONFIG_MONITOR_H
#define CAMERA_CONFIG_MONITOR_H

#include <functional>

// Callback type for bitdepth changes
typedef std::function<void(int)> BitdepthChangeCallback;

class CameraConfigMonitor
{
public:
    /**
     * Start the background monitor thread
     * Thread continuously checks for camera config changes in SHM
     * and applies them to PHD2's config when detected
     */
    static void Start();

    /**
     * Stop the monitor thread
     */
    static void Stop();
    
    /**
     * Register a callback to be called when bitdepth changes
     */
    static void SetBitdepthChangeCallback(BitdepthChangeCallback cb);
};

#endif // CAMERA_CONFIG_MONITOR_H
