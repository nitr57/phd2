/*
 *  shm_mount.cpp
 *  PHD Guiding
 *
 *  POSIX Shared Memory implementation for mount list (pure C, no wxWidgets)
 *  This file contains mount-specific implementations
 *
 */

#include "shm_mount.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include <stdio.h>

// File descriptor for the shared memory object
static int g_shm_fd = -1;
// Pointer to the mapped shared memory
static MountListSHM* g_shm_ptr = NULL;
// Size of the shared memory segment
static size_t g_shm_size = 0;
// Whether we own the shared memory (created it)
static int g_shm_owner = 0;

MountListSHM* shm_mount_init(int create_if_missing)
{
    if (g_shm_ptr != NULL)
    {
        // Already initialized
        return g_shm_ptr;
    }

    g_shm_size = sizeof(MountListSHM);

    // Try to open existing shared memory
    int shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        if (!create_if_missing)
        {
            return NULL;
        }

        // Create new shared memory
        shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1)
        {
            return NULL;
        }

        // Set the size of the shared memory
        if (ftruncate(shm_fd, g_shm_size) == -1)
        {
            close(shm_fd);
            shm_unlink(PHD2_MOUNT_SHM_NAME);
            return NULL;
        }

        g_shm_owner = 1;
    }

    // Map the shared memory
    void* ptr = mmap(NULL, g_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        close(shm_fd);
        if (g_shm_owner)
        {
            shm_unlink(PHD2_MOUNT_SHM_NAME);
        }
        return NULL;
    }

    g_shm_fd = shm_fd;
    g_shm_ptr = (MountListSHM*)ptr;

    // If we created it, initialize the structure
    if (g_shm_owner)
    {
        memset(g_shm_ptr, 0, g_shm_size);
        g_shm_ptr->version = PHD2_MOUNT_SHM_VERSION;
        g_shm_ptr->num_mounts = 0;
        g_shm_ptr->selected_mount_index = INVALID_MOUNT_INDEX;
        g_shm_ptr->timestamp = (uint32_t)time(NULL);
        g_shm_ptr->list_update_counter = 0;
        g_shm_ptr->selected_change_counter = 0;
    }

    return g_shm_ptr;
}

void shm_mount_cleanup(MountListSHM* shm, int unlink)
{
    if (shm == NULL)
    {
        return;
    }

    if (g_shm_ptr != NULL && g_shm_size > 0)
    {
        munmap(g_shm_ptr, g_shm_size);
        g_shm_ptr = NULL;
    }

    if (g_shm_fd >= 0)
    {
        close(g_shm_fd);
        g_shm_fd = -1;
    }

    if (unlink && g_shm_owner)
    {
        shm_unlink(PHD2_MOUNT_SHM_NAME);
    }

    g_shm_owner = 0;
    g_shm_size = 0;
}

int shm_mount_update_list(MountListSHM* shm, const char** mounts, uint32_t num_mounts)
{
    if (shm == NULL)
    {
        return -1;
    }

    if (num_mounts > MAX_MOUNTS_SHM)
    {
        return -1;
    }

    // Update the mount list
    shm->num_mounts = num_mounts;

    for (uint32_t i = 0; i < num_mounts; i++)
    {
        if (mounts[i] == NULL)
        {
            continue;
        }

        size_t len = strlen(mounts[i]);
        if (len >= MAX_MOUNT_NAME_LEN)
        {
            len = MAX_MOUNT_NAME_LEN - 1;
        }

        strncpy(shm->mounts[i].name, mounts[i], len);
        shm->mounts[i].name[len] = '\0';
    }

    // Clear remaining entries
    for (uint32_t i = num_mounts; i < MAX_MOUNTS_SHM; i++)
    {
        shm->mounts[i].name[0] = '\0';
    }

    // If the previously selected mount is no longer in the list, deselect it
    if (shm->selected_mount_index != INVALID_MOUNT_INDEX && shm->selected_mount_index >= num_mounts)
    {
        shm->selected_mount_index = INVALID_MOUNT_INDEX;
    }

    // Update metadata
    shm->timestamp = (uint32_t)time(NULL);
    shm->list_update_counter++;

    return 0;
}

int shm_mount_set_selected(MountListSHM* shm, uint32_t index)
{
    if (shm == NULL)
    {
        return -1;
    }

    // Validate index
    if (index != INVALID_MOUNT_INDEX && index >= shm->num_mounts)
    {
        return -1;
    }

    if (shm->selected_mount_index != index)
    {
        shm->selected_mount_index = index;
        shm->selected_change_counter++;
        shm->timestamp = (uint32_t)time(NULL);
    }

    return 0;
}

uint32_t shm_mount_get_selected(const MountListSHM* shm)
{
    if (shm == NULL)
    {
        return INVALID_MOUNT_INDEX;
    }

    uint32_t result = shm->selected_mount_index;
    return result;
}

int shm_mount_read_list(char mounts[][MAX_MOUNT_NAME_LEN], uint32_t max_mounts)
{
    const MountListSHM* shm = shm_mount_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    uint32_t num_to_read = shm->num_mounts;
    if (num_to_read > max_mounts)
    {
        num_to_read = max_mounts;
    }

    for (uint32_t i = 0; i < num_to_read; i++)
    {
        strncpy(mounts[i], shm->mounts[i].name, MAX_MOUNT_NAME_LEN - 1);
        mounts[i][MAX_MOUNT_NAME_LEN - 1] = '\0';
    }

    shm_mount_release_readonly(shm);

    return (int)num_to_read;
}

int shm_mount_read_selected(uint32_t* selected_index)
{
    const MountListSHM* shm = shm_mount_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    *selected_index = shm->selected_mount_index;

    shm_mount_release_readonly(shm);

    return 0;
}

int shm_mount_write_selected(uint32_t index)
{
    // Try to open existing shared memory for read-write
    int shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        return -1;
    }

    size_t shm_size = sizeof(MountListSHM);

    // Map read-write
    void* ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        close(shm_fd);
        return -1;
    }

    MountListSHM* shm = (MountListSHM*)ptr;

    // Validate index
    if (index != INVALID_MOUNT_INDEX && index >= shm->num_mounts)
    {
        munmap(shm, shm_size);
        close(shm_fd);
        return -1;
    }

    // Update selected mount
    if (shm->selected_mount_index != index)
    {
        shm->selected_mount_index = index;
        shm->selected_change_counter++;
        shm->timestamp = (uint32_t)time(NULL);
    }

    munmap(shm, shm_size);
    close(shm_fd);

    return 0;
}

const MountListSHM* shm_mount_get_readonly(void)
{
    // If already mapped in this process, return existing pointer
    if (g_shm_ptr != NULL)
    {
        return g_shm_ptr;
    }

    // Try to open existing shared memory (read-only for external processes)
    int shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_RDONLY, 0666);

    if (shm_fd == -1)
    {
        return NULL;
    }

    size_t shm_size = sizeof(MountListSHM);

    // Map read-only
    void* ptr = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);

    return (const MountListSHM*)ptr;
}

void shm_mount_release_readonly(const MountListSHM* shm)
{
    // Only unmap if it's not the global pointer (i.e., it's a temporary mapping)
    if (shm != NULL && shm != (const MountListSHM*)g_shm_ptr)
    {
        munmap((void*)shm, sizeof(MountListSHM));
    }
}

void shm_mount_signal_list_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_LIST_CHANGED, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

void shm_mount_signal_selected_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_SELECTED_CHANGED, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

int shm_mount_wait_list_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_LIST_CHANGED, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}

int shm_mount_wait_selected_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_SELECTED_CHANGED, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}

void shm_mount_signal_client_request(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_CLIENT_REQUEST, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

int shm_mount_wait_client_request(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_CLIENT_REQUEST, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    // Use timed wait with 1 second timeout to allow thread to check for shutdown
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 second timeout

    int result = sem_timedwait(sem, &ts);
    sem_close(sem);
    return result;
}
