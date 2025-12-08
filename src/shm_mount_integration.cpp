/*
 *  shm_mount_integration.cpp
 *  PHD Guiding
 *
 *  Integration wrapper for mount list shared memory
 *
 */

#include "phd.h"
#include "shm_mount_integration.h"
#include "shm_guider.h"

// Static member initialization
unsigned int MountSHMManager::s_last_change_counter = 0;

// Global pointer to shared memory
static EquipmentListSHM* g_mount_shm = NULL;

bool MountSHMManager::Initialize(void)
{
    if (g_mount_shm != NULL)
    {
        return true; // Already initialized
    }

    g_mount_shm = shm_mount_init(1); // Create if missing
    if (g_mount_shm == NULL)
    {
        Debug.Write("MountSHMManager: Failed to initialize shared memory\n");
        return false;
    }

    Debug.Write("MountSHMManager: Shared memory initialized\n");
    return true;
}

void MountSHMManager::Shutdown(void)
{
    if (g_mount_shm != NULL)
    {
        shm_mount_cleanup(g_mount_shm, 1); // Unlink on cleanup
        g_mount_shm = NULL;
        Debug.Write("MountSHMManager: Shared memory shut down\n");
    }
}

bool MountSHMManager::UpdateMountList(const wxArrayString& mounts)
{
    if (g_mount_shm == NULL)
    {
        Debug.Write("MountSHMManager: SHM not initialized\n");
        return false;
    }

    // Convert wxArrayString to C-style array
    const char** mount_names = new const char*[mounts.Count()];

    for (size_t i = 0; i < mounts.Count(); i++)
    {
        mount_names[i] = mounts[i].c_str();
    }

    if (shm_mount_update_list(g_mount_shm, mount_names, mounts.Count()) != 0)
    {
        Debug.Write("MountSHMManager: Failed to update mount list\n");
        delete[] mount_names;
        return false;
    }

    delete[] mount_names;

    Debug.Write(wxString::Format("MountSHMManager: Updated mount list with %zu mounts\n", mounts.Count()));
    return true;
}

int MountSHMManager::GetMountList(wxArrayString& mounts)
{
    if (g_mount_shm == NULL)
    {
        return 0;
    }

    mounts.Clear();

    for (uint32_t i = 0; i < g_mount_shm->num_items; i++)
    {
        mounts.Add(wxString(g_mount_shm->items[i].name));
    }

    return (int)g_mount_shm->num_items;
}

bool MountSHMManager::SetSelectedMount(int index)
{
    if (g_mount_shm == NULL)
    {
        Debug.Write("MountSHMManager: SHM not initialized\n");
        return false;
    }

    uint32_t shm_index = (index < 0) ? INVALID_ITEM_INDEX : (uint32_t)index;

    if (shm_mount_set_selected(g_mount_shm, shm_index) != 0)
    {
        Debug.Write(wxString::Format("MountSHMManager: Failed to set selected mount: %d\n", index));
        return false;
    }

    Debug.Write(wxString::Format("MountSHMManager: Selected mount index: %d\n", index));
    return true;
}

int MountSHMManager::GetSelectedMount(void)
{
    if (g_mount_shm == NULL)
    {
        return -1;
    }

    uint32_t index = shm_mount_get_selected(g_mount_shm);

    if (index == INVALID_ITEM_INDEX)
    {
        return -1;
    }

    return (int)index;
}

bool MountSHMManager::HasSelectionChanged(void)
{
    if (g_mount_shm == NULL)
    {
        return false;
    }

    bool changed = (g_mount_shm->selected_change_counter != s_last_change_counter);
    s_last_change_counter = g_mount_shm->selected_change_counter;

    return changed;
}
