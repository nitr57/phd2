/*
 *  shm_monitor.h
 *  PHD Guiding
 *
 *  Global shared memory monitor for headless and GUI modes
 *
 */

#ifndef SHM_MONITOR_H
#define SHM_MONITOR_H

class SHMMonitor
{
public:
    static bool Start();
    static void Stop();
    static bool IsRunning();
};

#endif // SHM_MONITOR_H
