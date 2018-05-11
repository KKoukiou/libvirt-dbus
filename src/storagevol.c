#include "storagevol.h"
#include "util.h"

#include <libvirt/libvirt.h>

static virtDBusGDBusPropertyTable virtDBusStorageVolPropertyTable[] = {
    { 0 }
};

static virtDBusGDBusMethodTable virtDBusStorageVolMethodTable[] = {
    { 0 }
};

static gchar **
virtDBusStorageVolEnumerate(gpointer userData)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virStoragePoolPtr) storagePools = NULL;
    gint numPools = 0;
    gint numVols = 0;
    gint numVolsTotal = 0;
    gint offset = 0;
    gchar **ret = NULL;

    if (!virtDBusConnectOpen(connect, NULL))
        return NULL;

    numPools = virConnectListAllStoragePools(connect->connection,
                                             &storagePools, 0);
    if (numPools < 0)
        return NULL;

    if (numPools == 0)
        return NULL;

    for (gint i = 0; i < numPools; i++) {
        g_autoptr(virStorageVolPtr) storageVols = NULL;

        numVols = virStoragePoolListAllVolumes(storagePools[i],
                                               &storageVols, 0);
        if (numVols < 0)
            return NULL;

        if (numVols == 0)
            return NULL;

        numVolsTotal += numVols;
    }

    ret = g_new0(gchar *, numVolsTotal + 1);

    for (gint i = 0; i < numPools; i++) {
        g_autoptr(virStorageVolPtr) storageVols = NULL;

        numVols = virStoragePoolListAllVolumes(storagePools[i],
                                               &storageVols, 0);
        if (numVols < 0)
            return NULL;

        if (numVols == 0)
            return NULL;

        for (gint j = 0; j < numVols; j++) {
            ret[offset] = virtDBusUtilBusPathForVirStorageVol(storageVols[j],
                                                              connect->storageVolPath);
            offset++;
        }
    }

    return ret;
}

static GDBusInterfaceInfo *interfaceInfo;

void
virtDBusStorageVolRegister(virtDBusConnect *connect,
                           GError **error)
{
    connect->storageVolPath = g_strdup_printf("%s/storagevol",
                                              connect->connectPath);

    if (!interfaceInfo) {
        interfaceInfo = virtDBusGDBusLoadIntrospectData(VIRT_DBUS_STORAGEVOL_INTERFACE,
                                                        error);
        if (!interfaceInfo)
            return;
    }

    virtDBusGDBusRegisterSubtree(connect->bus,
                                 connect->storageVolPath,
                                 interfaceInfo,
                                 virtDBusStorageVolEnumerate,
                                 virtDBusStorageVolMethodTable,
                                 virtDBusStorageVolPropertyTable,
                                 connect);
}
