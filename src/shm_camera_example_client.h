/*
 *  shm_camera_example_client.h
 *  PHD Guiding
 *
 *  Header for example client - can be used by other processes
 *
 */

#ifndef SHM_CAMERA_EXAMPLE_CLIENT_H
#define SHM_CAMERA_EXAMPLE_CLIENT_H

#include "shm_camera.h"

/**
 * Simple C API for external processes to interact with camera SHM
 * No dependencies on PHD2 or wxWidgets
 */

/* List available cameras */
void client_list_cameras(void);

/* Get currently selected camera */
void client_get_selected(void);

/* Set selected camera */
void client_set_selected(int index);

/* Monitor changes */
void client_monitor_changes(void);

#endif
