/*
 *  shm_camera_config.c
 *  PHD Guiding
 *
 *  POSIX Shared Memory for camera configuration
 *
 */

#include "shm_camera_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_CAMERA_CONFIG_NAME "/phd2_camera_config"
#define SHM_CAMERA_CONFIG_SIZE sizeof(CameraConfigSHM)

static int shm_camera_config_fd = -1;
static CameraConfigSHM* shm_camera_config_ptr = NULL;

CameraConfigSHM* shm_camera_config_init(int create)
{
    int flags = O_RDWR;
    if (create) {
        flags |= O_CREAT;
    }

    shm_camera_config_fd = shm_open(SHM_CAMERA_CONFIG_NAME, flags, 0666);
    if (shm_camera_config_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // If creating, resize to fit structure
    if (create) {
        struct stat sb;
        if (fstat(shm_camera_config_fd, &sb) == -1) {
            perror("fstat");
            close(shm_camera_config_fd);
            return NULL;
        }

        if (sb.st_size == 0) {
            // New SHM, need to resize
            if (ftruncate(shm_camera_config_fd, SHM_CAMERA_CONFIG_SIZE) == -1) {
                perror("ftruncate");
                close(shm_camera_config_fd);
                return NULL;
            }
        }
    }

    // Map the shared memory
    shm_camera_config_ptr = (CameraConfigSHM*) mmap(NULL, SHM_CAMERA_CONFIG_SIZE,
                                                      PROT_READ | PROT_WRITE,
                                                      MAP_SHARED,
                                                      shm_camera_config_fd,
                                                      0);

    if (shm_camera_config_ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_camera_config_fd);
        return NULL;
    }

    // Initialize header if creating
    if (create && shm_camera_config_ptr->magic != SHM_CAMERA_CONFIG_MAGIC) {
        memset(shm_camera_config_ptr, 0, SHM_CAMERA_CONFIG_SIZE);
        shm_camera_config_ptr->magic = SHM_CAMERA_CONFIG_MAGIC;
        shm_camera_config_ptr->version = 1;
        shm_camera_config_ptr->num_options = 0;
        shm_camera_config_ptr->update_counter = 0;
    }

    return shm_camera_config_ptr;
}

void shm_camera_config_cleanup(CameraConfigSHM* shm, int unlink)
{
    if (shm && shm != MAP_FAILED) {
        munmap(shm, SHM_CAMERA_CONFIG_SIZE);
    }

    if (shm_camera_config_fd >= 0) {
        close(shm_camera_config_fd);
        shm_camera_config_fd = -1;
    }

    if (unlink) {
        shm_unlink(SHM_CAMERA_CONFIG_NAME);
    }

    shm_camera_config_ptr = NULL;
}

const CameraConfigSHM* shm_camera_config_get_readonly(void)
{
    // If we already have it mapped (from init), just return that
    if (shm_camera_config_ptr) {
        return shm_camera_config_ptr;
    }

    // Try to open existing SHM without creating (read-only)
    int fd = shm_open(SHM_CAMERA_CONFIG_NAME, O_RDONLY, 0);
    if (fd == -1) {
        return NULL;
    }

    static CameraConfigSHM* readonly_ptr = NULL;
    
    // Check if already mapped
    if (readonly_ptr) {
        close(fd);
        return readonly_ptr;
    }

    CameraConfigSHM* ptr = (CameraConfigSHM*) mmap(NULL, SHM_CAMERA_CONFIG_SIZE,
                                                     PROT_READ,
                                                     MAP_SHARED,
                                                     fd,
                                                     0);
    close(fd);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    readonly_ptr = ptr;
    return ptr;
}

void shm_camera_config_release_readonly(const CameraConfigSHM* shm)
{
    // No-op: we cache the readonly mapping now
    (void)shm;  // Unused parameter
}

int shm_camera_config_set_option(CameraConfigSHM* shm, const char* option_name, int value)
{
    if (!shm || !option_name) {
        return -1;
    }

    // Find or create option
    int idx = -1;
    for (uint32_t i = 0; i < shm->num_options; i++) {
        if (strcmp(shm->options[i].name, option_name) == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        // Create new option
        if (shm->num_options >= SHM_CAMERA_CONFIG_MAX_OPTIONS) {
            return -1;  // No space for new option
        }
        idx = shm->num_options;
        shm->num_options++;
        memset(&shm->options[idx], 0, sizeof(CameraConfigOption));
        strncpy(shm->options[idx].name, option_name, SHM_CAMERA_CONFIG_OPTION_NAME_LEN - 1);
        shm->options[idx].min_value = 0;
        shm->options[idx].max_value = 255;
    }

    shm->options[idx].value = value;
    shm->update_counter++;
    
    // Signal the semaphore to wake up waiting monitors
    sem_t *sem = sem_open(SHM_CAMERA_CONFIG_SEM_NAME, O_CREAT, 0666, 0);
    if (sem != SEM_FAILED) {
        sem_post(sem);
        sem_close(sem);
    }

    return 0;
}

int shm_camera_config_get_option(const char* option_name, int* value)
{
    const CameraConfigSHM* shm = shm_camera_config_get_readonly();
    if (!shm || !option_name || !value) {
        return -1;
    }

    for (uint32_t i = 0; i < shm->num_options; i++) {
        if (strcmp(shm->options[i].name, option_name) == 0) {
            *value = shm->options[i].value;
            return 0;
        }
    }

    return -1;  // Option not found
}
