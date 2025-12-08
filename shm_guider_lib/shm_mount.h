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
 *  shm_mount.h
 *  PHD Guiding
 *
 *  POSIX Shared Memory interface for guider equipment (camera, mount, etc)
 *  Allows other processes to read available equipment and change selections
 *
 */

#ifndef SHM_MOUNT_H_INCLUDED
#define SHM_MOUNT_H_INCLUDED

#include <stdint.h>

// Maximum number of items that can be shared
#define MAX_ITEMS_SHM 64
// Maximum length of item name
#define MAX_ITEM_NAME_LEN 256

// Shared memory segment names
#define PHD2_CAMERA_SHM_NAME "/phd2_cameras"
#define PHD2_MOUNT_SHM_NAME "/phd2_mounts"

// POSIX semaphore names for camera events
#define PHD2_CAMERA_SEM_LIST_CHANGED "/phd2_cam_list_changed"
#define PHD2_CAMERA_SEM_SELECTED_CHANGED "/phd2_cam_selected_changed"
#define PHD2_CAMERA_SEM_CLIENT_REQUEST "/phd2_cam_client_request"

// POSIX semaphore names for mount events
#define PHD2_MOUNT_SEM_LIST_CHANGED "/phd2_mount_list_changed"
#define PHD2_MOUNT_SEM_SELECTED_CHANGED "/phd2_mount_selected_changed"
#define PHD2_MOUNT_SEM_CLIENT_REQUEST "/phd2_mount_client_request"

// Version for compatibility checking
#define PHD2_SHM_VERSION 1
// Invalid item index
#define INVALID_ITEM_INDEX 0xFFFFFFFF

#pragma pack(push, 1)

/**
 * Structure representing a single equipment entry in shared memory
 */
typedef struct {
    char name[MAX_ITEM_NAME_LEN];  // Equipment name/identifier
} EquipmentEntry;

/**
 * Main shared memory structure for camera list
 * This structure is mapped into POSIX shared memory for inter-process communication
 */
typedef struct {
    uint32_t version;                   // Version of this structure
    uint32_t num_items;                 // Number of items currently available
    uint32_t selected_index;            // Index of currently selected item (INVALID_ITEM_INDEX = none)
    uint32_t timestamp;                 // Timestamp of last update (seconds)
    uint32_t list_update_counter;       // Counter incremented when list changes
    uint32_t selected_change_counter;   // Counter incremented when selection changes
    uint8_t reserved[40];               // Reserved for future expansion
    EquipmentEntry items[MAX_ITEMS_SHM];
} EquipmentListSHM;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

// ===== MOUNT-SPECIFIC FUNCTIONS =====

/**
 * Initialize shared memory for mount list
 * Should be called by PHD2 process to create/open SHM
 * @param create_if_missing If true, create SHM if it doesn't exist
 * @return Pointer to shared memory structure, or NULL on error
 */
EquipmentListSHM* shm_mount_init(int create_if_missing);

/**
 * Cleanup mount shared memory resources
 * Should be called when exiting
 * @param shm Pointer to shared memory structure
 * @param unlink If true, unlink (delete) the shared memory
 */
void shm_mount_cleanup(EquipmentListSHM* shm, int unlink);

/**
 * Update the mount list in shared memory
 * @param shm Pointer to shared memory structure
 * @param mounts Array of mount names
 * @param num_mounts Number of mounts in array
 * @return 0 on success, -1 on error
 */
int shm_mount_update_list(EquipmentListSHM* shm, const char** mounts, uint32_t num_mounts);

/**
 * Set the selected mount index
 * @param shm Pointer to shared memory structure
 * @param index Index of mount to select (INVALID_ITEM_INDEX to deselect)
 * @return 0 on success, -1 on error (invalid index)
 */
int shm_mount_set_selected(EquipmentListSHM* shm, uint32_t index);

/**
 * Get the selected mount index
 * @param shm Pointer to shared memory structure
 * @return Selected mount index, or INVALID_ITEM_INDEX if none selected
 */
uint32_t shm_mount_get_selected(const EquipmentListSHM* shm);

/**
 * Read mount list from shared memory (for external processes)
 * @param mounts Output array to store mount names
 * @param max_mounts Maximum number of mounts to read
 * @return Number of mounts read, or -1 on error
 */
int shm_mount_read_list(char mounts[][MAX_ITEM_NAME_LEN], uint32_t max_mounts);

/**
 * Read selected mount index from shared memory (for external processes)
 * @param selected_index Output pointer to store selected mount index
 * @return 0 on success, -1 on error
 */
int shm_mount_read_selected(uint32_t* selected_index);

/**
 * Write selected mount index to shared memory (for external processes)
 * @param index Index of mount to select
 * @return 0 on success, -1 on error
 */
int shm_mount_write_selected(uint32_t index);

/**
 * Get the shared memory structure for read-only access
 * @return Pointer to shared memory structure, or NULL on error
 */
const EquipmentListSHM* shm_mount_get_readonly(void);

/**
 * Release read-only access to shared memory
 */
void shm_mount_release_readonly(const EquipmentListSHM* shm);

/**
 * Signal that mount list has changed (called by PHD2)
 */
void shm_mount_signal_list_changed(void);

/**
 * Signal that selected mount has changed (called by PHD2)
 */
void shm_mount_signal_selected_changed(void);

/**
 * Wait for mount list change notification (called by clients)
 * @return 0 on success, -1 on error
 */
int shm_mount_wait_list_changed(void);

/**
 * Wait for selected mount change notification (called by clients)
 * @return 0 on success, -1 on error
 */
int shm_mount_wait_selected_changed(void);

/**
 * Signal PHD2 that client has requested a mount change (called by clients)
 */
void shm_mount_signal_client_request(void);

/**
 * Wait for client mount request (called by PHD2)
 * @return 0 on success, -1 on error
 */
int shm_mount_wait_client_request(void);

#ifdef __cplusplus
}
#endif

#endif // SHM_MOUNT_H_INCLUDED
