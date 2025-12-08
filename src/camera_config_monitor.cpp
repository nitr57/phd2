/*
 *  camera_config_monitor.cpp
 *  PHD Guiding
 *
 *  Monitor thread for camera configuration changes via SHM
 *
 */

#include "phd.h"
#include "camera_config_monitor.h"
#include "camera_config_manager.h"
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

static bool camera_config_monitor_running = false;
static pthread_t camera_config_monitor_thread;
static BitdepthChangeCallback bitdepth_change_callback = nullptr;

static void* camera_config_monitor_thread_func(void* arg)
{
    sem_t *sem = sem_open("/phd2_camera_config_sem", O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        Debug.Write("CameraConfigMonitor: Failed to open semaphore\n");
        return nullptr;
    }

    while (camera_config_monitor_running)
    {
        // Wait for semaphore signal (with timeout to allow clean shutdown)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;  // 1 second timeout
        
        int ret = sem_timedwait(sem, &ts);
        if (ret == -1) {
            // Timeout or error, continue to check running flag
            continue;
        }

        // Check for bitdepth configuration changes
        int new_bitdepth;
        if (CameraConfigManager::GetUpdatedOption("bitdepth", &new_bitdepth)) {
            Debug.Write(wxString::Format("CameraConfigMonitor: bitdepth changed to %d\n", new_bitdepth));
            
            // Call the registered callback safely on the main thread via CallAfter
            if (bitdepth_change_callback) {
                wxTheApp->CallAfter([new_bitdepth]() {
                    if (bitdepth_change_callback) {
                        bitdepth_change_callback(new_bitdepth);
                    }
                });
            }
        }
    }

    sem_close(sem);
    return nullptr;
}

void CameraConfigMonitor::Start()
{
    if (camera_config_monitor_running) {
        return;  // Already running
    }

    camera_config_monitor_running = true;
    pthread_create(&camera_config_monitor_thread, nullptr, camera_config_monitor_thread_func, nullptr);
}

void CameraConfigMonitor::Stop()
{
    if (!camera_config_monitor_running) {
        return;
    }

    camera_config_monitor_running = false;
    pthread_join(camera_config_monitor_thread, nullptr);
}

void CameraConfigMonitor::SetBitdepthChangeCallback(BitdepthChangeCallback cb)
{
    bitdepth_change_callback = cb;
}
