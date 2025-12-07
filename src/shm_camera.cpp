/*
 *  shm_camera.cpp
 *  PHD Guiding
 *
 *  POSIX Shared Memory implementation for camera list
 *
 */

#include "phd.h"
#include "shm_camera.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

// File descriptor for the shared memory object
static int g_shm_fd = -1;
// Pointer to the mapped shared memory
static CameraListSHM* g_shm_ptr = NULL;
// Size of the shared memory segment
static size_t g_shm_size = 0;
// Whether we own the shared memory (created it)
static int g_shm_owner = 0;

CameraListSHM* shm_camera_init(int create_if_missing)
{
    if (g_shm_ptr != NULL)
    {
        // Already initialized
        return g_shm_ptr;
    }

    g_shm_size = sizeof(CameraListSHM);

    // Try to open existing shared memory
    int shm_fd = shm_open(PHD2_CAMERA_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        if (!create_if_missing)
        {
            Debug.Write(wxString::Format("shm_camera: Failed to open shared memory: %s\n", strerror(errno)));
            return NULL;
        }

        // Create new shared memory
        shm_fd = shm_open(PHD2_CAMERA_SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1)
        {
            Debug.Write(wxString::Format("shm_camera: Failed to create shared memory: %s\n", strerror(errno)));
            return NULL;
        }

        // Set the size of the shared memory
        if (ftruncate(shm_fd, g_shm_size) == -1)
        {
            Debug.Write(wxString::Format("shm_camera: Failed to set size: %s\n", strerror(errno)));
            close(shm_fd);
            shm_unlink(PHD2_CAMERA_SHM_NAME);
            return NULL;
        }

        g_shm_owner = 1;
    }

    // Map the shared memory
    void* ptr = mmap(NULL, g_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        Debug.Write(wxString::Format("shm_camera: Failed to map shared memory: %s\n", strerror(errno)));
        close(shm_fd);
        if (g_shm_owner)
        {
            shm_unlink(PHD2_CAMERA_SHM_NAME);
        }
        return NULL;
    }

    g_shm_fd = shm_fd;
    g_shm_ptr = (CameraListSHM*)ptr;

    // If we created it, initialize the structure
    if (g_shm_owner)
    {
        memset(g_shm_ptr, 0, g_shm_size);
        g_shm_ptr->version = PHD2_CAMERA_SHM_VERSION;
        g_shm_ptr->num_cameras = 0;
        g_shm_ptr->selected_camera_index = INVALID_CAMERA_INDEX;
        g_shm_ptr->timestamp = (uint32_t)time(NULL);
        g_shm_ptr->list_update_counter = 0;
        g_shm_ptr->selected_change_counter = 0;
        Debug.Write("shm_camera: Created and initialized shared memory\n");
    }
    else
    {
        Debug.Write("shm_camera: Opened existing shared memory\n");
    }

    return g_shm_ptr;
}

void shm_camera_cleanup(CameraListSHM* shm, int unlink)
{
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
        shm_unlink(PHD2_CAMERA_SHM_NAME);
        Debug.Write("shm_camera: Unlinked shared memory\n");
    }

    g_shm_owner = 0;
    g_shm_size = 0;
}

int shm_camera_update_list(CameraListSHM* shm, const char** cameras, uint32_t num_cameras)
{
    if (shm == NULL || g_shm_ptr == NULL)
    {
        return -1;
    }

    if (num_cameras > MAX_CAMERAS_SHM)
    {
        Debug.Write(wxString::Format("shm_camera: Too many cameras (%u > %d)\n", num_cameras, MAX_CAMERAS_SHM));
        return -1;
    }

    // Update the camera list
    for (uint32_t i = 0; i < num_cameras; i++)
    {
        if (cameras[i] == NULL)
        {
            continue;
        }

        size_t len = strlen(cameras[i]);
        if (len >= MAX_CAMERA_NAME_LEN)
        {
            Debug.Write(wxString::Format("shm_camera: Camera name too long: %s\n", cameras[i]));
            len = MAX_CAMERA_NAME_LEN - 1;
        }

        strncpy(g_shm_ptr->cameras[i].name, cameras[i], len);
        g_shm_ptr->cameras[i].name[len] = '\0';
    }

    // Clear remaining entries
    for (uint32_t i = num_cameras; i < MAX_CAMERAS_SHM; i++)
    {
        g_shm_ptr->cameras[i].name[0] = '\0';
    }

    // If the previously selected camera is no longer in the list, deselect it
    if (g_shm_ptr->selected_camera_index != INVALID_CAMERA_INDEX && g_shm_ptr->selected_camera_index >= num_cameras)
    {
        g_shm_ptr->selected_camera_index = INVALID_CAMERA_INDEX;
    }

    // Update metadata
    g_shm_ptr->num_cameras = num_cameras;
    g_shm_ptr->timestamp = (uint32_t)time(NULL);
    g_shm_ptr->list_update_counter++;

    return 0;
}

int shm_camera_set_selected(CameraListSHM* shm, uint32_t index)
{
    if (shm == NULL || g_shm_ptr == NULL)
    {
        return -1;
    }

    // Validate index
    if (index != INVALID_CAMERA_INDEX && index >= g_shm_ptr->num_cameras)
    {
        Debug.Write(wxString::Format("shm_camera: Invalid camera index: %u (max: %u)\n", index, g_shm_ptr->num_cameras - 1));
        return -1;
    }

    if (g_shm_ptr->selected_camera_index != index)
    {
        g_shm_ptr->selected_camera_index = index;
        g_shm_ptr->selected_change_counter++;
        g_shm_ptr->timestamp = (uint32_t)time(NULL);
    }

    return 0;
}

uint32_t shm_camera_get_selected(const CameraListSHM* shm)
{
    if (shm == NULL || g_shm_ptr == NULL)
    {
        return INVALID_CAMERA_INDEX;
    }

    return g_shm_ptr->selected_camera_index;
}

int shm_camera_read_list(char cameras[][MAX_CAMERA_NAME_LEN], uint32_t max_cameras)
{
    const CameraListSHM* shm = shm_camera_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    uint32_t num_to_read = shm->num_cameras;
    if (num_to_read > max_cameras)
    {
        num_to_read = max_cameras;
    }

    for (uint32_t i = 0; i < num_to_read; i++)
    {
        strncpy(cameras[i], shm->cameras[i].name, MAX_CAMERA_NAME_LEN - 1);
        cameras[i][MAX_CAMERA_NAME_LEN - 1] = '\0';
    }

    shm_camera_release_readonly(shm);

    return (int)num_to_read;
}

int shm_camera_read_selected(uint32_t* selected_index)
{
    const CameraListSHM* shm = shm_camera_get_readonly();

    if (shm == NULL)
    {
        return -1;
    }

    *selected_index = shm->selected_camera_index;

    shm_camera_release_readonly(shm);

    return 0;
}

int shm_camera_write_selected(uint32_t index)
{
    // Try to open existing shared memory for read-write
    int shm_fd = shm_open(PHD2_CAMERA_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        Debug.Write(wxString::Format("shm_camera: Failed to open shared memory for writing: %s\n", strerror(errno)));
        return -1;
    }

    size_t shm_size = sizeof(CameraListSHM);

    // Map read-write
    void* ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        Debug.Write(wxString::Format("shm_camera: Failed to map shared memory for writing: %s\n", strerror(errno)));
        close(shm_fd);
        return -1;
    }

    CameraListSHM* shm = (CameraListSHM*)ptr;

    // Validate index
    if (index != INVALID_CAMERA_INDEX && index >= shm->num_cameras)
    {
        Debug.Write(wxString::Format("shm_camera: Invalid camera index: %u (max: %u)\n", index, shm->num_cameras - 1));
        munmap(shm, shm_size);
        close(shm_fd);
        return -1;
    }

    // Update selected camera
    if (shm->selected_camera_index != index)
    {
        shm->selected_camera_index = index;
        shm->selected_change_counter++;
        shm->timestamp = (uint32_t)time(NULL);
    }

    munmap(shm, shm_size);
    close(shm_fd);

    return 0;
}

const CameraListSHM* shm_camera_get_readonly(void)
{
    // If already mapped in this process, return existing pointer
    if (g_shm_ptr != NULL)
    {
        return g_shm_ptr;
    }

    // Try to open existing shared memory (read-only for external processes)
    int shm_fd = shm_open(PHD2_CAMERA_SHM_NAME, O_RDONLY, 0666);

    if (shm_fd == -1)
    {
        Debug.Write(wxString::Format("shm_camera: Failed to open shared memory for reading: %s\n", strerror(errno)));
        return NULL;
    }

    size_t shm_size = sizeof(CameraListSHM);

    // Map read-only
    void* ptr = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);

    if (ptr == MAP_FAILED)
    {
        Debug.Write(wxString::Format("shm_camera: Failed to map shared memory for reading: %s\n", strerror(errno)));
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);

    return (const CameraListSHM*)ptr;
}

void shm_camera_release_readonly(const CameraListSHM* shm)
{
    // Only unmap if it's not the global pointer (i.e., it's a temporary mapping)
    if (shm != NULL && shm != (const CameraListSHM*)g_shm_ptr)
    {
        munmap((void*)shm, sizeof(CameraListSHM));
    }
}

void shm_camera_signal_list_changed(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_LIST_CHANGED, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

void shm_camera_signal_selected_changed(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_SELECTED_CHANGED, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

int shm_camera_wait_list_changed(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_LIST_CHANGED, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}

int shm_camera_wait_selected_changed(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_SELECTED_CHANGED, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}

void shm_camera_signal_client_request(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_CLIENT_REQUEST, 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
    }
}

int shm_camera_wait_client_request(void)
{
    sem_t* sem = sem_open(PHD2_CAMERA_SEM_CLIENT_REQUEST, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED)
    {
        return -1;
    }

    int result = sem_wait(sem);
    sem_close(sem);
    return result;
}
