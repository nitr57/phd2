/*
 *  shm_camera_example_client.c
 *  PHD Guiding
 *
 *  Example client program showing how to read/write camera list from shared memory
 *  This is a simple standalone program that doesn't depend on PHD2 libraries
 *
 *  Compile with: gcc -o shm_camera_client shm_camera_example_client.c -lrt
 *
 */

#include "shm_camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_camera_list(void)
{
    printf("\nAvailable cameras:\n");

    char cameras[MAX_CAMERAS_SHM][MAX_CAMERA_NAME_LEN];
    int num = shm_camera_read_list(cameras, MAX_CAMERAS_SHM);

    if (num < 0)
    {
        printf("  Error reading camera list\n");
        return;
    }

    if (num == 0)
    {
        printf("  (No cameras available)\n");
        return;
    }

    for (int i = 0; i < num; i++)
    {
        printf("  [%d] %s\n", i, cameras[i]);
    }
}

void print_selected_camera(void)
{
    uint32_t selected;

    if (shm_camera_read_selected(&selected) != 0)
    {
        printf("Error reading selected camera\n");
        return;
    }

    if (selected == INVALID_CAMERA_INDEX)
    {
        printf("No camera selected\n");
    }
    else
    {
        printf("Selected camera index: %u\n", selected);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <command> [args]\n", argv[0]);
        printf("Commands:\n");
        printf("  list          - List available cameras\n");
        printf("  selected      - Show selected camera index\n");
        printf("  select <idx>  - Select camera by index\n");
        printf("  deselect      - Deselect camera\n");
        printf("  monitor       - Monitor for changes (updates every second)\n");
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "list") == 0)
    {
        print_camera_list();
    }
    else if (strcmp(cmd, "selected") == 0)
    {
        print_selected_camera();
    }
    else if (strcmp(cmd, "select") == 0)
    {
        if (argc < 3)
        {
            printf("Error: select requires camera index\n");
            return 1;
        }

        int index = atoi(argv[2]);
        if (shm_camera_write_selected((uint32_t)index) == 0)
        {
            printf("Selected camera %d\n", index);
            // Signal PHD2 that a client has requested a change
            shm_camera_signal_client_request();
        }
        else
        {
            printf("Error selecting camera %d\n", index);
        }
    }
    else if (strcmp(cmd, "deselect") == 0)
    {
        if (shm_camera_write_selected(INVALID_CAMERA_INDEX) == 0)
        {
            printf("Deselected camera\n");
            // Signal PHD2 that a client has requested a change
            shm_camera_signal_client_request();
        }
        else
        {
            printf("Error deselecting camera\n");
        }
    }
    else if (strcmp(cmd, "monitor") == 0)
    {
        printf("Monitoring camera list (press Ctrl+C to stop)...\n\n");

        while (1)
        {
            printf("Waiting for changes...\n");

            // Wait for either list or selection change
            // In a real app, you might use select() or similar to wait on multiple semaphores
            // For now, we'll just wait on selected_changed
            if (shm_camera_wait_selected_changed() == 0)
            {
                uint32_t selected;
                if (shm_camera_read_selected(&selected) == 0)
                {
                    if (selected == INVALID_CAMERA_INDEX)
                    {
                        printf("Selection changed: No camera selected\n");
                    }
                    else
                    {
                        const CameraListSHM* shm = shm_camera_get_readonly();
                        if (shm && selected < shm->num_cameras)
                        {
                            printf("Selection changed: Camera %u (%s)\n", selected, shm->cameras[selected].name);
                        }
                    }
                }
            }
        }
    }
    else
    {
        printf("Unknown command: %s\n", cmd);
        return 1;
    }

    return 0;
}
