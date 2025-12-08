/*
 *  shm_mount.cpp
 *  PHD Guiding
 *
 *  POSIX Shared Memory implementation for mount equipment
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

// ===== MOUNT SHARED MEMORY IMPLEMENTATION =====

static int g_mount_shm_fd = -1;
static EquipmentListSHM* g_mount_shm_ptr = NULL;
static size_t g_mount_shm_size = 0;
static int g_mount_shm_owner = 0;

EquipmentListSHM* shm_mount_init(int create_if_missing)
{
    if (g_mount_shm_ptr != NULL)
    {
        return g_mount_shm_ptr;  // Already initialized
    }

    g_mount_shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_RDWR, 0666);

    if (g_mount_shm_fd == -1)
    {
        if (!create_if_missing)
        {
            fprintf(stderr, "shm_guider: Failed to open mount shared memory: %s\n", strerror(errno));
            return NULL;
        }

        g_mount_shm_fd = shm_open(PHD2_MOUNT_SHM_NAME, O_CREAT | O_RDWR, 0666);

        if (g_mount_shm_fd == -1)
        {
            fprintf(stderr, "shm_guider: Failed to create mount shared memory: %s\n", strerror(errno));
            return NULL;
        }

        g_mount_shm_size = sizeof(EquipmentListSHM);

        if (ftruncate(g_mount_shm_fd, g_mount_shm_size) == -1)
        {
            fprintf(stderr, "shm_guider: Failed to set size: %s\n", strerror(errno));
            close(g_mount_shm_fd);
            g_mount_shm_fd = -1;
            return NULL;
        }

        g_mount_shm_owner = 1;
    }
    else
    {
        g_mount_shm_owner = 0;

        struct stat sb;
        if (fstat(g_mount_shm_fd, &sb) == -1)
        {
            fprintf(stderr, "shm_guider: Failed to stat mount SHM: %s\n", strerror(errno));
            close(g_mount_shm_fd);
            g_mount_shm_fd = -1;
            return NULL;
        }
        g_mount_shm_size = sb.st_size;
    }

    g_mount_shm_ptr = (EquipmentListSHM*)mmap(NULL, g_mount_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_mount_shm_fd, 0);

    if (g_mount_shm_ptr == MAP_FAILED)
    {
        fprintf(stderr, "shm_guider: Failed to map mount shared memory: %s\n", strerror(errno));
        close(g_mount_shm_fd);
        g_mount_shm_fd = -1;
        g_mount_shm_ptr = NULL;
        return NULL;
    }

    // Initialize structure if we created it
    if (g_mount_shm_owner)
    {
        memset(g_mount_shm_ptr, 0, g_mount_shm_size);
        g_mount_shm_ptr->version = PHD2_SHM_VERSION;
        g_mount_shm_ptr->selected_index = INVALID_ITEM_INDEX;

        fprintf(stderr, "shm_guider: Created and initialized mount shared memory\n");
    }
    else
    {
        fprintf(stderr, "shm_guider: Opened existing mount shared memory\n");
    }

    return g_mount_shm_ptr;
}

void shm_mount_cleanup(EquipmentListSHM* shm, int unlink)
{
    if (shm == NULL)
        return;

    if (shm == g_mount_shm_ptr)
    {
        if (g_mount_shm_ptr != NULL)
        {
            munmap(g_mount_shm_ptr, g_mount_shm_size);
            g_mount_shm_ptr = NULL;
        }

        if (g_mount_shm_fd != -1)
        {
            close(g_mount_shm_fd);
            g_mount_shm_fd = -1;
        }

        if (unlink && g_mount_shm_owner)
        {
            shm_unlink(PHD2_MOUNT_SHM_NAME);
            fprintf(stderr, "shm_guider: Unlinked mount shared memory\n");
        }
    }
}

int shm_mount_update_list(EquipmentListSHM* shm, const char** mounts, uint32_t num_mounts)
{
    if (shm == NULL)
        return -1;

    if (num_mounts > MAX_ITEMS_SHM)
    {
        fprintf(stderr, "shm_guider: Too many mounts (%u > %d)\n", num_mounts, MAX_ITEMS_SHM);
        return -1;
    }

    shm->num_items = num_mounts;

    for (uint32_t i = 0; i < num_mounts; i++)
    {
        if (mounts[i] == NULL)
        {
            fprintf(stderr, "shm_guider: NULL mount name at index %u\n", i);
            return -1;
        }

        size_t len = strlen(mounts[i]);
        if (len >= MAX_ITEM_NAME_LEN)
        {
            fprintf(stderr, "shm_guider: Mount name too long: %s\n", mounts[i]);
            return -1;
        }

        strncpy(shm->items[i].name, mounts[i], MAX_ITEM_NAME_LEN - 1);
        shm->items[i].name[MAX_ITEM_NAME_LEN - 1] = '\0';
    }

    shm->timestamp = (uint32_t)time(NULL);
    shm->list_update_counter++;

    shm_mount_signal_list_changed();

    return 0;
}

int shm_mount_set_selected(EquipmentListSHM* shm, uint32_t index)
{
    if (shm == NULL)
        return -1;

    if (index != INVALID_ITEM_INDEX && index >= shm->num_items)
    {
        fprintf(stderr, "shm_guider: Invalid mount index: %u (max: %u)\n", index, shm->num_items - 1);
        return -1;
    }

    shm->selected_index = index;
    shm->timestamp = (uint32_t)time(NULL);
    shm->selected_change_counter++;

    shm_mount_signal_selected_changed();

    return 0;
}

uint32_t shm_mount_get_selected(const EquipmentListSHM* shm)
{
    if (shm == NULL)
        return INVALID_ITEM_INDEX;

    return shm->selected_index;
}

int shm_mount_read_list(char mounts[][MAX_ITEM_NAME_LEN], uint32_t max_mounts)
{
    const EquipmentListSHM* shm = shm_mount_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    uint32_t num_to_read = shm->num_items;
    if (num_to_read > max_mounts)
    {
        num_to_read = max_mounts;
    }

    for (uint32_t i = 0; i < num_to_read; i++)
    {
        strncpy(mounts[i], shm->items[i].name, MAX_ITEM_NAME_LEN - 1);
        mounts[i][MAX_ITEM_NAME_LEN - 1] = '\0';
    }

    shm_mount_release_readonly(shm);

    return (int)num_to_read;
}

int shm_mount_read_selected(uint32_t* selected_index)
{
    const EquipmentListSHM* shm = shm_mount_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    *selected_index = shm->selected_index;
    shm_mount_release_readonly(shm);

    return 0;
}

int shm_mount_write_selected(uint32_t index)
{
    EquipmentListSHM* shm = shm_mount_init(0);

    if (shm == NULL)
    {
        return -1;
    }

    return shm_mount_set_selected(shm, index);
}

const EquipmentListSHM* shm_mount_get_readonly(void)
{
    if (g_mount_shm_ptr != NULL)
    {
        return g_mount_shm_ptr;
    }

    EquipmentListSHM* shm = shm_mount_init(0);

    return shm;
}

void shm_mount_release_readonly(const EquipmentListSHM* shm)
{
    // Read-only access, nothing to do
    (void)shm;
}

void shm_mount_signal_list_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_LIST_CHANGED, O_CREAT, 0666, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

void shm_mount_signal_selected_changed(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_SELECTED_CHANGED, O_CREAT, 0666, 0);
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

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 second timeout

    int result = sem_timedwait(sem, &ts);
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

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 second timeout

    int result = sem_timedwait(sem, &ts);
    sem_close(sem);
    return result;
}

void shm_mount_signal_client_request(void)
{
    sem_t* sem = sem_open(PHD2_MOUNT_SEM_CLIENT_REQUEST, O_CREAT, 0666, 0);
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

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 second timeout

    int result = sem_timedwait(sem, &ts);
    sem_close(sem);
    return result;
}
