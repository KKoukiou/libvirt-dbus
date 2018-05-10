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
virtDBusEventsDomainBalloonChange(virConnectPtr connection G_GNUC_UNUSED,
                                  virDomainPtr domain,
                                  guint64 actual,
                                  gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "BalloonChange",
                                  g_variant_new("(t)", actual),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainBlockJob(virConnectPtr connection G_GNUC_UNUSED,
                             virDomainPtr domain,
                             gchar *disk,
                             gint type,
                             gint status,
                             gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "BlockJob",
                                  g_variant_new("(suu)", disk, type, status),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainControlError(virConnectPtr connection G_GNUC_UNUSED,
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
                                  "ControlError",
                                  NULL,
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
virtDBusEventsDomainDeviceRemovalFailed(virConnectPtr connection G_GNUC_UNUSED,
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
                                  "DeviceRemovalFailed",
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
virtDBusEventsDomainGraphics(virConnectPtr connection G_GNUC_UNUSED,
                             virDomainPtr domain,
                             gint phase,
                             const virDomainEventGraphicsAddress *local,
                             const virDomainEventGraphicsAddress *remote,
                             const gchar *authScheme,
                             const virDomainEventGraphicsSubject *subject,
                             gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;
    GVariantBuilder builder;
    GVariant *gret;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_variant_builder_init(&builder, G_VARIANT_TYPE("(i(iss)(iss)sa(ss))"));

    g_variant_builder_add(&builder, "i", phase);

    g_variant_builder_open(&builder, G_VARIANT_TYPE("(iss)"));
    g_variant_builder_add(&builder, "i", local->family);
    g_variant_builder_add(&builder, "s", local->node);
    g_variant_builder_add(&builder, "s", local->service);
    g_variant_builder_close(&builder);

    g_variant_builder_open(&builder, G_VARIANT_TYPE("(iss)"));
    g_variant_builder_add(&builder, "i", remote->family);
    g_variant_builder_add(&builder, "s", remote->node);
    g_variant_builder_add(&builder, "s", remote->service);
    g_variant_builder_close(&builder);

    g_variant_builder_add(&builder, "s", authScheme);

    g_variant_builder_open(&builder, G_VARIANT_TYPE("a(ss)"));
    for (gint i = 0; i < subject->nidentity; i++) {
        g_variant_builder_open(&builder, G_VARIANT_TYPE("(ss)"));
        g_variant_builder_add(&builder, "ss", subject->identities[i].type);
        g_variant_builder_add(&builder, "ss", subject->identities[i].name);
        g_variant_builder_close(&builder);
    }
    g_variant_builder_close(&builder);
    gret = g_variant_builder_end(&builder);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "Graphics",
                                  gret,
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainIOError(virConnectPtr connection G_GNUC_UNUSED,
                            virDomainPtr domain,
                            const gchar *srcPath,
                            const gchar *device,
                            gint action,
                            gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "IOError",
                                  g_variant_new("(ssu)", srcPath, device,
                                                action),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainIOErrorReason(virConnectPtr connection G_GNUC_UNUSED,
                                  virDomainPtr domain,
                                  const gchar *srcPath,
                                  const gchar *device,
                                  gint action,
                                  const gchar *reason,
                                  gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "IOErrorReason",
                                  g_variant_new("(ssus)", srcPath, device,
                                                action, reason),
                                  NULL);

    return 0;
}

static gint
virtDBusEventsDomainJobCompleted(virConnectPtr connection G_GNUC_UNUSED,
                                 virDomainPtr domain,
                                 virTypedParameterPtr params,
                                 gint nparams,
                                 gpointer opaque)
{
    virtDBusConnect *connect = opaque;
    g_autofree gchar *path = NULL;
    GVariant *gargs;

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    gargs = virtDBusUtilTypedParamsToGVariant(params, nparams);

    g_dbus_connection_emit_signal(connect->bus,
                                  NULL,
                                  path,
                                  VIRT_DBUS_DOMAIN_INTERFACE,
                                  "JobCompleted",
                                  g_variant_new_tuple(&gargs, 1),
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
                                      VIR_DOMAIN_EVENT_ID_BALLOON_CHANGE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainBalloonChange));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_BLOCK_JOB,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainBlockJob));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_BLOCK_JOB_2,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainBlockJob));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_CONTROL_ERROR,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainControlError));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_LIFECYCLE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainLifecycle));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DEVICE_ADDED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDeviceAdded));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DEVICE_REMOVAL_FAILED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDeviceRemovalFailed));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDeviceRemoved));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_DISK_CHANGE,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainDiskChange));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_GRAPHICS,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainGraphics));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_IO_ERROR,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainIOError));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_IO_ERROR_REASON,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainIOErrorReason));

    virtDBusEventsRegisterDomainEvent(connect,
                                      VIR_DOMAIN_EVENT_ID_JOB_COMPLETED,
                                      VIR_DOMAIN_EVENT_CALLBACK(virtDBusEventsDomainJobCompleted));

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
