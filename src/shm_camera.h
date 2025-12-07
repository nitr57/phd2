/*
 *  shm_camera.h
 *  PHD Guiding
 *
 *  POSIX Shared Memory interface for camera list
 *  Allows other processes to read available cameras and change the selected camera
 *
 */

#ifndef SHM_CAMERA_H_INCLUDED
#define SHM_CAMERA_H_INCLUDED

#include <stdint.h>

// Maximum number of cameras that can be shared
#define MAX_CAMERAS_SHM 64
// Maximum length of camera name
#define MAX_CAMERA_NAME_LEN 256
// Shared memory segment name
#define PHD2_CAMERA_SHM_NAME "/phd2_cameras"
// POSIX semaphore names for event signaling
#define PHD2_CAMERA_SEM_LIST_CHANGED "/phd2_cam_list_changed"
#define PHD2_CAMERA_SEM_SELECTED_CHANGED "/phd2_cam_selected_changed"
#define PHD2_CAMERA_SEM_CLIENT_REQUEST "/phd2_cam_client_request"
// Version for compatibility checking
#define PHD2_CAMERA_SHM_VERSION 1
// Invalid camera index
#define INVALID_CAMERA_INDEX 0xFFFFFFFF

#pragma pack(push, 1)

/**
 * Structure representing a single camera entry in shared memory
 */
typedef struct {
    char name[MAX_CAMERA_NAME_LEN];  // Camera name/identifier
} CameraEntry;

/**
 * Main shared memory structure containing camera list and selected camera
 * This structure is mapped into POSIX shared memory for inter-process communication
 */
typedef struct {
    uint32_t version;                   // Version of this structure
    uint32_t num_cameras;               // Number of cameras currently available
    uint32_t selected_camera_index;     // Index of currently selected camera (INVALID_CAMERA_INDEX = none)
    uint32_t timestamp;                 // Timestamp of last update (seconds)
    uint32_t list_update_counter;       // Counter incremented when camera list changes
    uint32_t selected_change_counter;   // Counter incremented when selected camera changes
    uint8_t reserved[40];               // Reserved for future expansion
    CameraEntry cameras[MAX_CAMERAS_SHM];
} CameraListSHM;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize shared memory for camera list
 * Should be called by PHD2 process to create/open SHM
 * @param create_if_missing If true, create SHM if it doesn't exist
 * @return Pointer to shared memory structure, or NULL on error
 */
CameraListSHM* shm_camera_init(int create_if_missing);

/**
 * Cleanup shared memory resources
 * Should be called when exiting
 * @param shm Pointer to shared memory structure
 * @param unlink If true, unlink (delete) the shared memory
 */
void shm_camera_cleanup(CameraListSHM* shm, int unlink);

/**
 * Update the camera list in shared memory
 * @param shm Pointer to shared memory structure
 * @param cameras Array of camera names
 * @param num_cameras Number of cameras in array
 * @return 0 on success, -1 on error
 */
int shm_camera_update_list(CameraListSHM* shm, const char** cameras, uint32_t num_cameras);

/**
 * Set the selected camera index
 * @param shm Pointer to shared memory structure
 * @param index Index of camera to select (INVALID_CAMERA_INDEX to deselect)
 * @return 0 on success, -1 on error (invalid index)
 */
int shm_camera_set_selected(CameraListSHM* shm, uint32_t index);

/**
 * Get the selected camera index
 * @param shm Pointer to shared memory structure
 * @return Selected camera index, or INVALID_CAMERA_INDEX if none selected
 */
uint32_t shm_camera_get_selected(const CameraListSHM* shm);

/**
 * Read camera list from shared memory (for external processes)
 * @param cameras Output array to store camera names
 * @param max_cameras Maximum number of cameras to read
 * @return Number of cameras read, or -1 on error
 */
int shm_camera_read_list(char cameras[][MAX_CAMERA_NAME_LEN], uint32_t max_cameras);

/**
 * Read selected camera index from shared memory (for external processes)
 * @param selected_index Output pointer to store selected camera index
 * @return 0 on success, -1 on error
 */
int shm_camera_read_selected(uint32_t* selected_index);

/**
 * Write selected camera index to shared memory (for external processes)
 * @param index Index of camera to select
 * @return 0 on success, -1 on error
 */
int shm_camera_write_selected(uint32_t index);

/**
 * Get the shared memory structure for read-only access
 * @return Pointer to shared memory structure, or NULL on error
 */
const CameraListSHM* shm_camera_get_readonly(void);

/**
 * Release read-only access to shared memory
 */
void shm_camera_release_readonly(const CameraListSHM* shm);

/**
 * Signal that camera list has changed (called by PHD2)
 * Notifies all waiting clients
 */
void shm_camera_signal_list_changed(void);

/**
 * Signal that selected camera has changed (called by PHD2)
 * Notifies all waiting clients
 */
void shm_camera_signal_selected_changed(void);

/**
 * Wait for camera list change notification (called by clients)
 * Blocks until list changes
 * @return 0 on success, -1 on error
 */
int shm_camera_wait_list_changed(void);

/**
 * Wait for selected camera change notification (called by clients)
 * Blocks until selected camera changes
 * @return 0 on success, -1 on error
 */
int shm_camera_wait_selected_changed(void);

/**
 * Signal PHD2 that client has requested a change (called by clients)
 * Use this after writing a new selected camera index to SHM
 */
void shm_camera_signal_client_request(void);

/**
 * Wait for client request (called by PHD2)
 * Blocks until a client signals a request
 * @return 0 on success, -1 on error
 */
int shm_camera_wait_client_request(void);

#ifdef __cplusplus
}
#endif

#endif // SHM_CAMERA_H_INCLUDED
