#include "domain.h"
#include "events.h"
#include "util.h"
#include "storagepool.h"

#include <libvirt/libvirt.h>

static gint
virtDBusEventsDomainAgentLifecycle(virConnectPtr connection G_GNUC_UNUSED,
                                   virDomainPtr domain,
                                   guint state,
                                   guint reason,
                                   gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "AgentLifecycle",
                                  g_variant_new("(uu)", state, reason),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainLifecycle(virConnectPtr connection G_GNUC_UNUSED,
                              virDomainPtr domain,
                              gint event,
                              gint detail,
                              gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  connect->connectPath,
                                  VIRT_DBUS_CONNECT_INTERFACE,
                                  "DomainEvent",
                                  g_variant_new("(ouu)", path, event, detail),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainDeviceAdded(virConnectPtr connection G_GNUC_UNUSED,
                                virDomainPtr domain,
                                const gchar *device,
                                gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "DeviceAdded",
                                  g_variant_new("(s)", device),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainDeviceRemoved(virConnectPtr connection G_GNUC_UNUSED,
                                  virDomainPtr domain,
                                  const gchar *device,
                                  gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "DeviceRemoved",
                                  g_variant_new("(s)", device),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainTrayChange(virConnectPtr connection G_GNUC_UNUSED,
                               virDomainPtr domain,
                               const gchar *device,
                               gint reason,
                               gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "TrayChange",
                                  g_variant_new("(su)", device, reason),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainDiskChange(virConnectPtr connection G_GNUC_UNUSED,
                               virDomainPtr domain,
                               const gchar *old_src_path,
                               const gchar *new_src_path,
                               const gchar *device,
                               gint reason,
                               gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "DiskChange",
                                  g_variant_new("(sssu)", old_src_path,
                                                new_src_path, device, reason),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainReboot(virConnectPtr connection G_GNUC_UNUSED,
                           virDomainPtr domain,
                           gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "Reboot",
                                  NULL,
                                  NULL);

    return 0;
}

static gint
virtDBusEventsNetworkLifecycle(virConnectPtr connection G_GNUC_UNUSED,
                               virNetworkPtr network,
                               gint event,
                               gint detail G_GNUC_UNUSED,
                               gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirNetwork(network, connect->networkPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  connect->connectPath,
                                  VIRT_DBUS_CONNECT_INTERFACE,
                                  "NetworkEvent",
                                  g_variant_new("(ou)", path, event),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsStoragePoolLifecycle(virConnectPtr connection G_GNUC_UNUSED,
                                   virStoragePoolPtr storagePool,
                                   gint event,
                                   gint detail,
                                   gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirStoragePool(storagePool,
                                                connect->storagePoolPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  connect->connectPath,
                                  VIRT_DBUS_CONNECT_INTERFACE,
                                  "StoragePoolEvent",
                                  g_variant_new("(ouu)", path, event, detail),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsStoragePoolRefresh(virConnectPtr connection G_GNUC_UNUSED,
                                 virStoragePoolPtr storagePool,
                                 gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirStoragePool(storagePool,
                                                connect->storagePoolPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_STORAGEPOOL_INTERFACE,
                                  "Refresh",
                                  NULL,
                                  NULL);

    return 0;
}

static void
virtDBusEventsRegisterDomainEvent(virtDBusConnect *connect,
                                  gint id,
                                  virConnectDomainEventGenericCallback callback)
{
    g_assert(connect->domainCallbackIds[id] == -1);

    connect->domainCallbackIds[id] = virConnectDomainEventRegisterAny(connect->connection,
                                                                      NULL,
                                                                      id,
                                                                      VIR_DOMAIN_EVENT_CALLBACK(callback),
                                                                      connect,
                                                                      NULL);
}

static void
virtDBusEventsRegisterNetworkEvent(virtDBusConnect *connect,
                                   gint id,
                                   virConnectNetworkEventGenericCallback callback)
{
    g_assert(connect->networkCallbackIds[id] == -1);

    connect->networkCallbackIds[id] = virConnectNetworkEventRegisterAny(connect->connection,
                                                                        NULL,
                                                                        id,
                                                                        VIR_NETWORK_EVENT_CALLBACK(callback),
                                                                        connect,
                                                                        NULL);
}

static void
virtDBusEventsRegisterStoragePoolEvent(virtDBusConnect *connect,
                                       gint id,
                                       virConnectStoragePoolEventGenericCallback callback)
{
    g_assert(connect->storagePoolCallbackIds[id] == -1);

    connect->storagePoolCallbackIds[id] = virConnectStoragePoolEventRegisterAny(connect->connection,
                                                                                NULL,
                                                                                id,
                                                                                VIR_STORAGE_POOL_EVENT_CALLBACK(callback),
                                                                                connect,
                                                                                NULL);
}

void
virtDBusEventsRegister(virtDBusConnect *connect)
{
    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_AGENT_LIFECYCLE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainAgentLifecycle));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_LIFECYCLE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainLifecycle));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DEVICE_ADDED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDeviceAdded));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDeviceRemoved));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DISK_CHANGE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDiskChange));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_REBOOT,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainReboot));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_TRAY_CHANGE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainTrayChange));

    virtDBusEventsRegisterNetworkEvent(connect,
                                       VIR_NETWORK_EVENT_ID_LIFECYCLE,
                                       VIR_NETWORK_EVENT_CALLBACK(virtDBusEventsNetworkLifecycle));

    virtDBusEventsRegisterStoragePoolEvent(connect,
                                           VIR_STORAGE_POOL_EVENT_ID_LIFECYCLE,
                                           VIR_STORAGE_POOL_EVENT_CALLBACK(virtDBusEventsStoragePoolLifecycle));

    virtDBusEventsRegisterStoragePoolEvent(connect,
                                           VIR_STORAGE_POOL_EVENT_ID_REFRESH,
                                           VIR_STORAGE_POOL_EVENT_CALLBACK(virtDBusEventsStoragePoolRefresh));
}
