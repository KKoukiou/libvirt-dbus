#include "connect.h"
#include "domain.h"
#include "events.h"
#include "network.h"
#include "util.h"

#include <glib/gprintf.h>

VIRT_DBUS_ENUM_DECL(virtDBusConnectCPUCompareResult)
VIRT_DBUS_ENUM_IMPL(virtDBusConnectCPUCompareResult,
                    VIR_CPU_COMPARE_LAST,
                    "incompatible",
                    "identical",
                    "superset")

VIRT_DBUS_ENUM_DECL(virtDBusConnectNodeSuspendTarget)
VIRT_DBUS_ENUM_IMPL(virtDBusConnectNodeSuspendTarget,
                    VIR_NODE_SUSPEND_TARGET_LAST,
                    "mem",
                    "disk",
                    "hybrid")

static gint virtDBusConnectCredType[] = {
    VIR_CRED_AUTHNAME,
    VIR_CRED_ECHOPROMPT,
    VIR_CRED_REALM,
    VIR_CRED_PASSPHRASE,
    VIR_CRED_NOECHOPROMPT,
    VIR_CRED_EXTERNAL,
};

static gint
virtDBusConnectAuthCallback(virConnectCredentialPtr cred G_GNUC_UNUSED,
                            guint ncred G_GNUC_UNUSED,
                            gpointer cbdata)
{
    GError **error = cbdata;
    g_set_error(error, VIRT_DBUS_ERROR, VIRT_DBUS_ERROR_LIBVIRT,
                "Interactive authentication is not supported. "
                "Use client configuration file for libvirt.");
    return -1;
}

static virConnectAuth virtDBusConnectAuth = {
    virtDBusConnectCredType,
    G_N_ELEMENTS(virtDBusConnectCredType),
    virtDBusConnectAuthCallback,
    NULL,
};

static void
virtDBusConnectClose(virtDBusConnect *connect,
                     gboolean deregisterEvents)
{

    for (gint i = 0; i < VIR_DOMAIN_EVENT_ID_LAST; i += 1) {
        if (connect->domainCallbackIds[i] >= 0) {
            if (deregisterEvents) {
                virConnectDomainEventDeregisterAny(connect->connection,
                                                   connect->domainCallbackIds[i]);
            }
            connect->domainCallbackIds[i] = -1;
        }
    }

    for (gint i = 0; i < VIR_NETWORK_EVENT_ID_LAST; i += 1) {
        if (connect->networkCallbackIds[i] >= 0) {
            if (deregisterEvents) {
                virConnectNetworkEventDeregisterAny(connect->connection,
                                                    connect->networkCallbackIds[i]);
            }
            connect->networkCallbackIds[i] = -1;
        }
    }

    virConnectClose(connect->connection);
    connect->connection = NULL;
}

gboolean
virtDBusConnectOpen(virtDBusConnect *connect,
                    GError **error)
{
    virtDBusUtilAutoLock lock = g_mutex_locker_new(&connect->lock);

    if (connect->connection) {
        if (virConnectIsAlive(connect->connection))
            return TRUE;
        else
            virtDBusConnectClose(connect, FALSE);
    }

    virtDBusConnectAuth.cbdata = error;

    connect->connection = virConnectOpenAuth(connect->uri,
                                             &virtDBusConnectAuth, 0);
    if (!connect->connection) {
        if (error && !*error)
            virtDBusUtilSetLastVirtError(error);
        return FALSE;
    }

    virtDBusEventsRegister(connect);

    return TRUE;
}

static void
virtDBusConnectGetEncrypted(const gchar *objectPath G_GNUC_UNUSED,
                            gpointer userData,
                            GVariant **value,
                            GError **error)
{
    virtDBusConnect *connect = userData;
    gint encrypted;

    if (!virtDBusConnectOpen(connect, error))
        return;

    encrypted = virConnectIsEncrypted(connect->connection);
    if (encrypted < 0)
        return virtDBusUtilSetLastVirtError(error);

    *value = g_variant_new("b", !!encrypted);
}

static void
virtDBusConnectGetHostname(const gchar *objectPath G_GNUC_UNUSED,
                           gpointer userData,
                           GVariant **value,
                           GError **error)
{
    virtDBusConnect *connect = userData;
    g_autofree gchar * hostname = NULL;

    if (!virtDBusConnectOpen(connect, error))
        return;

    hostname = virConnectGetHostname(connect->connection);
    if (!hostname)
        return virtDBusUtilSetLastVirtError(error);

    *value = g_variant_new("s", hostname);
}

static void
virtDBusConnectGetLibVersion(const gchar *objectPath G_GNUC_UNUSED,
                             gpointer userData,
                             GVariant **value,
                             GError **error)
{
    virtDBusConnect *connect = userData;
    gulong libVer;

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virConnectGetLibVersion(connect->connection, &libVer) < 0)
        return virtDBusUtilSetLastVirtError(error);

    *value = g_variant_new("t", libVer);
}

static void
virtDBusConnectGetSecure(const gchar *objectPath G_GNUC_UNUSED,
                         gpointer userData,
                         GVariant **value,
                         GError **error)
{
    virtDBusConnect *connect = userData;
    gint secure;

    if (!virtDBusConnectOpen(connect, error))
        return;

    secure = virConnectIsEncrypted(connect->connection);
    if (secure < 0)
        return virtDBusUtilSetLastVirtError(error);

    *value = g_variant_new("b", !!secure);
}

static void
virtDBusConnectGetVersion(const gchar *objectPath G_GNUC_UNUSED,
                          gpointer userData,
                          GVariant **value,
                          GError **error)
{
    virtDBusConnect *connect = userData;
    gulong hvVer;

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virConnectGetVersion(connect->connection, &hvVer) < 0)
        return virtDBusUtilSetLastVirtError(error);

    *value = g_variant_new("t", hvVer);
}

static void
virtDBusConnectGetCapabilities(GVariant *inArgs G_GNUC_UNUSED,
                               GUnixFDList *inFDs G_GNUC_UNUSED,
                               const gchar *objectPath G_GNUC_UNUSED,
                               gpointer userData,
                               GVariant **outArgs,
                               GUnixFDList **outFDs G_GNUC_UNUSED,
                               GError **error)
{
    virtDBusConnect *connect = userData;
    g_autofree gchar *capabilities = NULL;

    if (!virtDBusConnectOpen(connect, error))
        return;

    capabilities = virConnectGetCapabilities(connect->connection);
    if (!capabilities)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(s)", capabilities);
}

static void
virtDBusConnectBaselineCPU(GVariant *inArgs,
                           GUnixFDList *inFDs G_GNUC_UNUSED,
                           const gchar *objectPath G_GNUC_UNUSED,
                           gpointer userData,
                           GVariant **outArgs,
                           GUnixFDList **outFDs G_GNUC_UNUSED,
                           GError **error)
{
    virtDBusConnect *connect = userData;
    g_autofree const gchar **xmlCPUs = NULL;
    g_autoptr(GVariantIter) iter = NULL;
    const gchar **tmp;
    gsize ncpus;
    g_autofree gchar *cpu = NULL;
    guint flags;

    g_variant_get(inArgs, "(asu)", &iter, &flags);

    ncpus = g_variant_iter_n_children(iter);
    if (ncpus > 0) {
        xmlCPUs = g_new0(const gchar*, ncpus);
        tmp = xmlCPUs;
        while (g_variant_iter_next(iter, "&s", tmp))
            tmp++;
    }

    if (!virtDBusConnectOpen(connect, error))
        return;

    cpu = virConnectBaselineCPU(connect->connection, xmlCPUs, ncpus, flags);
    if (!cpu)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(s)", cpu);
}

static void
virtDBusConnectCompareCPU(GVariant *inArgs,
                          GUnixFDList *inFDs G_GNUC_UNUSED,
                          const gchar *objectPath G_GNUC_UNUSED,
                          gpointer userData,
                          GVariant **outArgs,
                          GUnixFDList **outFDs G_GNUC_UNUSED,
                          GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *xmlDesc;
    guint flags;
    gint compareResult;
    const gchar* compareResultStr;

    g_variant_get(inArgs, "(&su)", &xmlDesc, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    compareResult = virConnectCompareCPU(connect->connection, xmlDesc, flags);
    if (compareResult < 0)
        return virtDBusUtilSetLastVirtError(error);

    compareResultStr = virtDBusConnectCPUCompareResultTypeToString(compareResult);
    if (!compareResultStr) {
        g_set_error(error, VIRT_DBUS_ERROR, VIRT_DBUS_ERROR_LIBVIRT,
                    "Can't format virCPUCompareResult '%d' to string.",
                    compareResult);
        return;
    }

    *outArgs = g_variant_new("(s)", compareResultStr);
}

static void
virtDBusConnectDomainCreateXML(GVariant *inArgs,
                               GUnixFDList *inFDs G_GNUC_UNUSED,
                               const gchar *objectPath G_GNUC_UNUSED,
                               gpointer userData,
                               GVariant **outArgs,
                               GUnixFDList **outFDs G_GNUC_UNUSED,
                               GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomain) domain = NULL;
    g_autofree gchar *path = NULL;
    gchar *xml;
    guint flags;

    g_variant_get(inArgs, "(&su)", &xml, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    domain = virDomainCreateXML(connect->connection, xml, flags);
    if (!domain)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectDomainDefineXML(GVariant *inArgs,
                               GUnixFDList *inFDs G_GNUC_UNUSED,
                               const gchar *objectPath G_GNUC_UNUSED,
                               gpointer userData,
                               GVariant **outArgs,
                               GUnixFDList **outFDs G_GNUC_UNUSED,
                               GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomain) domain = NULL;
    g_autofree gchar *path = NULL;
    gchar *xml;

    g_variant_get(inArgs, "(&s)", &xml);

    if (!virtDBusConnectOpen(connect, error))
        return;

    domain = virDomainDefineXML(connect->connection, xml);
    if (!domain)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectDomainLookupByID(GVariant *inArgs,
                                GUnixFDList *inFDs G_GNUC_UNUSED,
                                const gchar *objectPath G_GNUC_UNUSED,
                                gpointer userData,
                                GVariant **outArgs,
                                GUnixFDList **outFDs G_GNUC_UNUSED,
                                GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomain) domain = NULL;
    g_autofree gchar *path = NULL;
    guint id;

    g_variant_get(inArgs, "(u)", &id);

    if (!virtDBusConnectOpen(connect, NULL))
        return;

    domain = virDomainLookupByID(connect->connection, id);
    if (!domain)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectDomainLookupByName(GVariant *inArgs,
                                  GUnixFDList *inFDs G_GNUC_UNUSED,
                                  const gchar *objectPath G_GNUC_UNUSED,
                                  gpointer userData,
                                  GVariant **outArgs,
                                  GUnixFDList **outFDs G_GNUC_UNUSED,
                                  GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomain) domain = NULL;
    g_autofree gchar *path = NULL;
    const gchar *name;

    g_variant_get(inArgs, "(s)", &name);

    if (!virtDBusConnectOpen(connect, NULL))
        return;

    domain = virDomainLookupByName(connect->connection, name);
    if (!domain)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectDomainLookupByUUID(GVariant *inArgs,
                                  GUnixFDList *inFDs G_GNUC_UNUSED,
                                  const gchar *objectPath G_GNUC_UNUSED,
                                  gpointer userData,
                                  GVariant **outArgs,
                                  GUnixFDList **outFDs G_GNUC_UNUSED,
                                  GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomain) domain = NULL;
    g_autofree gchar *path = NULL;
    const gchar *uuidstr;

    g_variant_get(inArgs, "(s)", &uuidstr);

    if (!virtDBusConnectOpen(connect, NULL))
        return;

    domain = virDomainLookupByUUIDString(connect->connection, uuidstr);
    if (!domain)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirDomain(domain, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectDomainRestoreFlags(GVariant *inArgs,
                                  GUnixFDList *inFDs G_GNUC_UNUSED,
                                  const gchar *objectPath G_GNUC_UNUSED,
                                  gpointer userData,
                                  GVariant **outArgs G_GNUC_UNUSED,
                                  GUnixFDList **outFDs G_GNUC_UNUSED,
                                  GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *from;
    const gchar *xml;
    guint flags;

    g_variant_get(inArgs, "(&s&su)", &from, &xml, &flags);
    if (g_str_equal(xml, ""))
        xml = NULL;

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virDomainRestoreFlags(connect->connection, from, xml, flags) < 0)
        virtDBusUtilSetLastVirtError(error);
}

static void
virtDBusConnectDomainSaveImageDefineXML(GVariant *inArgs,
                                        GUnixFDList *inFDs G_GNUC_UNUSED,
                                        const gchar *objectPath G_GNUC_UNUSED,
                                        gpointer userData,
                                        GVariant **outArgs G_GNUC_UNUSED,
                                        GUnixFDList **outFDs G_GNUC_UNUSED,
                                        GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *file;
    const gchar *xml;
    guint flags;

    g_variant_get(inArgs, "(&s&su)", &file, &xml, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virDomainSaveImageDefineXML(connect->connection, file, xml, flags) < 0)
        virtDBusUtilSetLastVirtError(error);
}

static void
virtDBusConnectFindStoragePoolSources(GVariant *inArgs,
                                      GUnixFDList *inFDs G_GNUC_UNUSED,
                                      const gchar *objectPath G_GNUC_UNUSED,
                                      gpointer userData,
                                      GVariant **outArgs,
                                      GUnixFDList **outFDs G_GNUC_UNUSED,
                                      GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *type;
    const gchar *srcSpec;
    guint flags;
    g_autofree gchar *ret = NULL;

    g_variant_get(inArgs, "(&s&su)", &type, &srcSpec, &flags);
    if (g_str_equal(srcSpec, ""))
        srcSpec = NULL;

    if (!virtDBusConnectOpen(connect, error))
        return;

    ret = virConnectFindStoragePoolSources(connect->connection, type, srcSpec, flags);
    if (!ret)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(s)", ret);
}

static void
virtDBusConnectGetAllDomainStats(GVariant *inArgs,
                                 GUnixFDList *inFDs G_GNUC_UNUSED,
                                 const gchar *objectPath G_GNUC_UNUSED,
                                 gpointer userData,
                                 GVariant **outArgs,
                                 GUnixFDList **outFDs G_GNUC_UNUSED,
                                 GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomainStatsRecordPtr) records = NULL;
    guint stats;
    gint nstats;
    guint flags;
    GVariant *gret;
    GVariantBuilder builder;

    g_variant_get(inArgs, "(uu)", &stats, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    nstats = virConnectGetAllDomainStats(connect->connection,
                                         stats, &records, flags);
    if (nstats < 0)
        return virtDBusUtilSetLastVirtError(error);

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sa{sv})"));

    for (gint i = 0; i < nstats; i++) {
        const gchar* name;
        GVariant *grecords;

        g_variant_builder_open(&builder, G_VARIANT_TYPE("(sa{sv})"));
        name = virDomainGetName(records[i]->dom);
        grecords = virtDBusUtilTypedParamsToGVariant(records[i]->params,
                                                     records[i]->nparams);
        g_variant_builder_add(&builder, "s", name);
        g_variant_builder_add_value(&builder, grecords);
        g_variant_builder_close(&builder);
    }
    gret = g_variant_builder_end(&builder);

    *outArgs = g_variant_new_tuple(&gret, 1);
}

static void
virtDBusConnectGetCPUModelNames(GVariant *inArgs,
                                GUnixFDList *inFDs G_GNUC_UNUSED,
                                const gchar *objectPath G_GNUC_UNUSED,
                                gpointer userData,
                                GVariant **outArgs,
                                GUnixFDList **outFDs G_GNUC_UNUSED,
                                GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *arch;
    guint flags;
    g_autoptr(virtDBusCharArray) models = NULL;
    gint nmodels;
    GVariant *gret;
    GVariantBuilder builder;

    g_variant_get(inArgs, "(&su)", &arch, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    nmodels = virConnectGetCPUModelNames(connect->connection, arch, &models, flags);
    if (nmodels < 0)
        return virtDBusUtilSetLastVirtError(error);

    g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
    for (gint i = 0; i < nmodels; i++)
        g_variant_builder_add(&builder, "s", models[i]);
    gret = g_variant_builder_end(&builder);

    *outArgs = g_variant_new_tuple(&gret, 1);
}

static void
virtDBusConnectGetDomainCapabilities(GVariant *inArgs,
                                     GUnixFDList *inFDs G_GNUC_UNUSED,
                                     const gchar *objectPath G_GNUC_UNUSED,
                                     gpointer userData,
                                     GVariant **outArgs,
                                     GUnixFDList **outFDs G_GNUC_UNUSED,
                                     GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *emulatorbin;
    const gchar *arch;
    const gchar *machine;
    const gchar *virttype;
    guint flags;
    g_autofree gchar* domCapabilities = NULL;

    g_variant_get(inArgs, "(&s&s&s&su)", &emulatorbin, &arch, &machine,
                  &virttype, &flags);
    if (g_str_equal(emulatorbin, ""))
        emulatorbin = NULL;
    if (g_str_equal(arch, ""))
        arch = NULL;
    if (g_str_equal(machine, ""))
        machine = NULL;
    if (g_str_equal(virttype, ""))
        virttype = NULL;

    if (!virtDBusConnectOpen(connect, error))
        return;

    domCapabilities = virConnectGetDomainCapabilities(connect->connection,
                                                      emulatorbin, arch,
                                                      machine, virttype,
                                                      flags);
    if (!domCapabilities)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(s)", domCapabilities);
}

static void
virtDBusConnectGetSysinfo(GVariant *inArgs,
                          GUnixFDList *inFDs G_GNUC_UNUSED,
                          const gchar *objectPath G_GNUC_UNUSED,
                          gpointer userData,
                          GVariant **outArgs,
                          GUnixFDList **outFDs G_GNUC_UNUSED,
                          GError **error)

{
    virtDBusConnect *connect = userData;
    guint flags;
    g_autofree gchar *sysinfo = NULL;

    g_variant_get(inArgs, "(u)", &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    sysinfo = virConnectGetSysinfo(connect->connection, flags);
    if (!sysinfo)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(s)", sysinfo);
}

static void
virtDBusConnectListDomains(GVariant *inArgs,
                           GUnixFDList *inFDs G_GNUC_UNUSED,
                           const gchar *objectPath G_GNUC_UNUSED,
                           gpointer userData,
                           GVariant **outArgs,
                           GUnixFDList **outFDs G_GNUC_UNUSED,
                           GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virDomainPtr) domains = NULL;
    guint flags;
    GVariantBuilder builder;
    GVariant *gdomains;

    g_variant_get(inArgs, "(u)", &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virConnectListAllDomains(connect->connection, &domains, flags) < 0)
        return virtDBusUtilSetLastVirtError(error);

    if (!*domains)
        return;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("ao"));

    for (gint i = 0; domains[i]; i++) {
        g_autofree gchar *path = NULL;
        path = virtDBusUtilBusPathForVirDomain(domains[i],
                                               connect->domainPath);

        g_variant_builder_add(&builder, "o", path);
    }

    gdomains = g_variant_builder_end(&builder);
    *outArgs = g_variant_new_tuple(&gdomains, 1);
}

static void
virtDBusConnectListNetworks(GVariant *inArgs,
                            GUnixFDList *inFDs G_GNUC_UNUSED,
                            const gchar *objectPath G_GNUC_UNUSED,
                            gpointer userData,
                            GVariant **outArgs,
                            GUnixFDList **outFDs G_GNUC_UNUSED,
                            GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virNetworkPtr) networks = NULL;
    guint flags;
    GVariantBuilder builder;
    GVariant *gnetworks;

    g_variant_get(inArgs, "(u)", &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virConnectListAllNetworks(connect->connection, &networks, flags) < 0)
        return virtDBusUtilSetLastVirtError(error);

    if (!*networks)
        return;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("ao"));

    for (gint i = 0; networks[i]; i++) {
        g_autofree gchar *path = NULL;
        path = virtDBusUtilBusPathForVirNetwork(networks[i],
                                                connect->networkPath);

        g_variant_builder_add(&builder, "o", path);
    }

    gnetworks = g_variant_builder_end(&builder);
    *outArgs = g_variant_new_tuple(&gnetworks, 1);
}

static void
virtDBusConnectNetworkCreateXML(GVariant *inArgs,
                                GUnixFDList *inFDs G_GNUC_UNUSED,
                                const gchar *objectPath G_GNUC_UNUSED,
                                gpointer userData,
                                GVariant **outArgs,
                                GUnixFDList **outFDs G_GNUC_UNUSED,
                                GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virNetwork) network = NULL;
    g_autofree gchar *path = NULL;
    const gchar *xml;

    g_variant_get(inArgs, "(&s)", &xml);

    if (!virtDBusConnectOpen(connect, error))
        return;

    network = virNetworkCreateXML(connect->connection, xml);
    if (!network)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirNetwork(network, connect->domainPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectNetworkDefineXML(GVariant *inArgs,
                                GUnixFDList *inFDs G_GNUC_UNUSED,
                                const gchar *objectPath G_GNUC_UNUSED,
                                gpointer userData,
                                GVariant **outArgs,
                                GUnixFDList **outFDs G_GNUC_UNUSED,
                                GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virNetwork) network = NULL;
    g_autofree gchar *path = NULL;
    const gchar *xml;

    g_variant_get(inArgs, "(&s)", &xml);

    if (!virtDBusConnectOpen(connect, error))
        return;

    network = virNetworkDefineXML(connect->connection, xml);
    if (!network)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirNetwork(network, connect->networkPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectNetworkLookupByName(GVariant *inArgs,
                                   GUnixFDList *inFDs G_GNUC_UNUSED,
                                   const gchar *objectPath G_GNUC_UNUSED,
                                   gpointer userData,
                                   GVariant **outArgs,
                                   GUnixFDList **outFDs G_GNUC_UNUSED,
                                   GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virNetwork) network = NULL;
    g_autofree gchar *path = NULL;
    const gchar *name;

    g_variant_get(inArgs, "(s)", &name);

    if (!virtDBusConnectOpen(connect, error))
        return;

    network = virNetworkLookupByName(connect->connection, name);
    if (!network)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirNetwork(network, connect->networkPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectNetworkLookupByUUID(GVariant *inArgs,
                                   GUnixFDList *inFDs G_GNUC_UNUSED,
                                   const gchar *objectPath G_GNUC_UNUSED,
                                   gpointer userData,
                                   GVariant **outArgs,
                                   GUnixFDList **outFDs G_GNUC_UNUSED,
                                   GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virNetwork) network = NULL;
    g_autofree gchar *path = NULL;
    const gchar *uuidstr;

    g_variant_get(inArgs, "(&s)", &uuidstr);

    if (!virtDBusConnectOpen(connect, error))
        return;

    network = virNetworkLookupByUUIDString(connect->connection, uuidstr);
    if (!network)
        return virtDBusUtilSetLastVirtError(error);

    path = virtDBusUtilBusPathForVirNetwork(network, connect->networkPath);

    *outArgs = g_variant_new("(o)", path);
}

static void
virtDBusConnectNodeGetCPUStats(GVariant *inArgs,
                               GUnixFDList *inFDs G_GNUC_UNUSED,
                               const gchar *objectPath G_GNUC_UNUSED,
                               gpointer userData,
                               GVariant **outArgs,
                               GUnixFDList **outFDs G_GNUC_UNUSED,
                               GError **error)
{
    virtDBusConnect *connect = userData;
    gint cpuNum;
    guint flags;
    g_autofree virNodeCPUStatsPtr stats = NULL;
    gint count = 0;
    gint ret;
    GVariant *gret;
    GVariantBuilder builder;

    g_variant_get(inArgs, "(iu)", &cpuNum, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    ret = virNodeGetCPUStats(connect->connection, cpuNum, NULL, &count, flags);
    if (ret == 0 && count != 0) {
        stats = g_new0(virNodeCPUStats, count);
        if (virNodeGetCPUStats(connect->connection, cpuNum, stats,
                               &count, flags) < 0) {
            return virtDBusUtilSetLastVirtError(error);
        }
    }

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{st}"));
    for (gint i = 0; i < count; i++)
        g_variant_builder_add(&builder, "{st}", stats[i].field, stats[i].value);
    gret = g_variant_builder_end(&builder);

    *outArgs = g_variant_new_tuple(&gret, 1);
}

static void
virtDBusConnectNodeGetFreeMemory(GVariant *inArgs G_GNUC_UNUSED,
                                 GUnixFDList *inFDs G_GNUC_UNUSED,
                                 const gchar *objectPath G_GNUC_UNUSED,
                                 gpointer userData,
                                 GVariant **outArgs,
                                 GUnixFDList **outFDs G_GNUC_UNUSED,
                                 GError **error)

{
    virtDBusConnect *connect = userData;
    guint64 freemem;

    if (!virtDBusConnectOpen(connect, error))
        return;

    freemem = virNodeGetFreeMemory(connect->connection);
    if (freemem == 0)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("(t)", freemem);
}

static void
virtDBusConnectNodeGetInfo(GVariant *inArgs G_GNUC_UNUSED,
                           GUnixFDList *inFDs G_GNUC_UNUSED,
                           const gchar *objectPath G_GNUC_UNUSED,
                           gpointer userData,
                           GVariant **outArgs,
                           GUnixFDList **outFDs G_GNUC_UNUSED,
                           GError **error)

{
    virtDBusConnect *connect = userData;
    virNodeInfo info;

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virNodeGetInfo(connect->connection, &info) < 0)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("((stuuuuuu))", info.model, info.memory,
                             info.cpus, info.mhz, info.nodes, info.sockets,
                             info.cores, info.threads);
}

static void
virtDBusConnectNodeGetMemoryParameters(GVariant *inArgs,
                                       GUnixFDList *inFDs G_GNUC_UNUSED,
                                       const gchar *objectPath G_GNUC_UNUSED,
                                       gpointer userData,
                                       GVariant **outArgs,
                                       GUnixFDList **outFDs G_GNUC_UNUSED,
                                       GError **error)
{
    virtDBusConnect *connect = userData;
    g_auto(virtDBusUtilTypedParams) params = { 0 };
    guint flags;
    gint ret;
    GVariant *grecords;

    g_variant_get(inArgs, "(u)", &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    ret = virNodeGetMemoryParameters(connect->connection, NULL,
                                     &params.nparams, flags);
    if (ret < 0)
        return virtDBusUtilSetLastVirtError(error);
    if (params.nparams != 0) {
        params.params = g_new0(virTypedParameter, params.nparams);
        if (virNodeGetMemoryParameters(connect->connection, params.params,
                                       &params.nparams, flags) < 0) {
            return virtDBusUtilSetLastVirtError(error);
        }
    }

    grecords = virtDBusUtilTypedParamsToGVariant(params.params, params.nparams);

    *outArgs = g_variant_new_tuple(&grecords, 1);
}

static void
virtDBusConnectNodeGetMemoryStats(GVariant *inArgs,
                                  GUnixFDList *inFDs G_GNUC_UNUSED,
                                  const gchar *objectPath G_GNUC_UNUSED,
                                  gpointer userData,
                                  GVariant **outArgs,
                                  GUnixFDList **outFDs G_GNUC_UNUSED,
                                  GError **error)
{
    virtDBusConnect *connect = userData;
    g_autofree virNodeMemoryStatsPtr params = NULL;
    gint nparams = 0;
    gint cellNum = VIR_NODE_MEMORY_STATS_ALL_CELLS;
    guint flags;
    gint ret;
    GVariantBuilder builder;
    GVariant *res;

    g_variant_get(inArgs, "(u)", &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    ret = virNodeGetMemoryStats(connect->connection, cellNum, NULL,
                                &nparams, flags);
    if (ret < 0)
        return virtDBusUtilSetLastVirtError(error);
    if (nparams != 0) {
        params = g_new0(virNodeMemoryStats, nparams);
        if (virNodeGetMemoryStats(connect->connection, cellNum, params,
                                  &nparams, flags) < 0) {
            return virtDBusUtilSetLastVirtError(error);
        }
    }

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{st}"));
    for (gint i = 0; i < nparams; i++)
        g_variant_builder_add(&builder, "{st}", params[i].field, params[i].value);
    res = g_variant_builder_end(&builder);

    *outArgs = g_variant_new_tuple(&res, 1);
}

static void
virtDBusConnectNodeGetSecurityModel(GVariant *inArgs G_GNUC_UNUSED,
                                    GUnixFDList *inFDs G_GNUC_UNUSED,
                                    const gchar *objectPath G_GNUC_UNUSED,
                                    gpointer userData,
                                    GVariant **outArgs,
                                    GUnixFDList **outFDs G_GNUC_UNUSED,
                                    GError **error)

{
    virtDBusConnect *connect = userData;
    virSecurityModel secmodel;

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (virNodeGetSecurityModel(connect->connection, &secmodel) < 0)
        return virtDBusUtilSetLastVirtError(error);

    *outArgs = g_variant_new("((ss))", secmodel.model, secmodel.doi);
}

static void
virtDBusConnectNodeSetMemoryParameters(GVariant *inArgs,
                                       GUnixFDList *inFDs G_GNUC_UNUSED,
                                       const gchar *objectPath G_GNUC_UNUSED,
                                       gpointer userData,
                                       GVariant **outArgs G_GNUC_UNUSED,
                                       GUnixFDList **outFDs G_GNUC_UNUSED,
                                       GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(GVariantIter) iter = NULL;
    g_auto(virtDBusUtilTypedParams) params = { 0 };
    guint flags;

    g_variant_get(inArgs, "(a{sv}u)", &iter, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    if (!virtDBusUtilGVariantToTypedParams(iter, &params.params,
                                           &params.nparams, error)) {
        return;
    }

    if (virNodeSetMemoryParameters(connect->connection, params.params,
                                   params.nparams, flags) < 0) {
        virtDBusUtilSetLastVirtError(error);
    }
}

static void
virtDBusConnectNodeSuspendForDuration(GVariant *inArgs,
                                      GUnixFDList *inFDs G_GNUC_UNUSED,
                                      const gchar *objectPath G_GNUC_UNUSED,
                                      gpointer userData,
                                      GVariant **outArgs G_GNUC_UNUSED,
                                      GUnixFDList **outFDs G_GNUC_UNUSED,
                                      GError **error)
{
    virtDBusConnect *connect = userData;
    const gchar *targetStr;
    gint target;
    guint64 duration;
    guint flags;

    g_variant_get(inArgs, "(&stu)", &targetStr, &duration, &flags);

    if (!virtDBusConnectOpen(connect, error))
        return;

    target = virtDBusConnectNodeSuspendTargetTypeFromString(targetStr);
    if (target < 0) {
        g_set_error(error, VIRT_DBUS_ERROR, VIRT_DBUS_ERROR_LIBVIRT,
                    "Can't get valid virNodeSuspendTarget from string '%s'.",
                    targetStr);
        return;
    }

    if (virNodeSuspendForDuration(connect->connection, target,
                                  duration, flags) < 0) {
        virtDBusUtilSetLastVirtError(error);
    }
}

static virtDBusGDBusPropertyTable virtDBusConnectPropertyTable[] = {
    { "Encrypted", virtDBusConnectGetEncrypted, NULL },
    { "Hostname", virtDBusConnectGetHostname, NULL },
    { "LibVersion", virtDBusConnectGetLibVersion, NULL },
    { "Secure", virtDBusConnectGetSecure, NULL },
    { "Version", virtDBusConnectGetVersion, NULL },
    { 0 }
};

static virtDBusGDBusMethodTable virtDBusConnectMethodTable[] = {
    { "BaselineCPU", virtDBusConnectBaselineCPU },
    { "CompareCPU", virtDBusConnectCompareCPU },
    { "DomainCreateXML", virtDBusConnectDomainCreateXML },
    { "DomainDefineXML", virtDBusConnectDomainDefineXML },
    { "DomainLookupByID", virtDBusConnectDomainLookupByID },
    { "DomainLookupByName", virtDBusConnectDomainLookupByName },
    { "DomainLookupByUUID", virtDBusConnectDomainLookupByUUID },
    { "DomainRestore", virtDBusConnectDomainRestoreFlags },
    { "DomainSaveImageDefineXML", virtDBusConnectDomainSaveImageDefineXML },
    { "FindStoragePoolSources", virtDBusConnectFindStoragePoolSources },
    { "GetAllDomainStats", virtDBusConnectGetAllDomainStats },
    { "GetCapabilities", virtDBusConnectGetCapabilities },
    { "GetCPUModelNames", virtDBusConnectGetCPUModelNames },
    { "GetDomainCapabilities", virtDBusConnectGetDomainCapabilities },
    { "GetSysinfo", virtDBusConnectGetSysinfo },
    { "ListDomains", virtDBusConnectListDomains },
    { "ListNetworks", virtDBusConnectListNetworks },
    { "NetworkCreateXML", virtDBusConnectNetworkCreateXML },
    { "NetworkDefineXML", virtDBusConnectNetworkDefineXML },
    { "NetworkLookupByName", virtDBusConnectNetworkLookupByName },
    { "NetworkLookupByUUID", virtDBusConnectNetworkLookupByUUID },
    { "NodeGetCPUStats", virtDBusConnectNodeGetCPUStats },
    { "NodeGetFreeMemory", virtDBusConnectNodeGetFreeMemory },
    { "NodeGetInfo", virtDBusConnectNodeGetInfo },
    { "NodeGetMemoryParameters", virtDBusConnectNodeGetMemoryParameters },
    { "NodeGetMemoryStats", virtDBusConnectNodeGetMemoryStats },
    { "NodeGetSecurityModel", virtDBusConnectNodeGetSecurityModel },
    { "NodeSetMemoryParameters", virtDBusConnectNodeSetMemoryParameters },
    { "NodeSuspendForDuration", virtDBusConnectNodeSuspendForDuration },
    { 0 }
};

static GDBusInterfaceInfo *interfaceInfo = NULL;

static void
virtDBusConnectFree(virtDBusConnect *connect)
{
    if (connect->connection)
        virtDBusConnectClose(connect, TRUE);

    g_free(connect->domainPath);
    g_free(connect->networkPath);
    g_free(connect);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC(virtDBusConnect, virtDBusConnectFree);

void
virtDBusConnectNew(virtDBusConnect **connectp,
                   GDBusConnection *bus,
                   const gchar *uri,
                   const gchar *connectPath,
                   GError **error)
{
    g_autoptr(virtDBusConnect) connect = NULL;

    if (!interfaceInfo) {
        interfaceInfo = virtDBusGDBusLoadIntrospectData(VIRT_DBUS_CONNECT_INTERFACE,
                                                        error);
        if (!interfaceInfo)
            return;
    }

    connect = g_new0(virtDBusConnect, 1);

    g_mutex_init(&connect->lock);

    for (gint i = 0; i < VIR_DOMAIN_EVENT_ID_LAST; i += 1)
        connect->domainCallbackIds[i] = -1;

    for (gint i = 0; i < VIR_NETWORK_EVENT_ID_LAST; i += 1)
        connect->networkCallbackIds[i] = -1;

    connect->bus = bus;
    connect->uri = uri;
    connect->connectPath = connectPath;

    virtDBusGDBusRegisterObject(bus,
                                connect->connectPath,
                                interfaceInfo,
                                virtDBusConnectMethodTable,
                                virtDBusConnectPropertyTable,
                                connect);

    virtDBusDomainRegister(connect, error);
    if (error && *error)
        return;

    virtDBusNetworkRegister(connect, error);
    if (error && *error)
        return;

    *connectp = connect;
    connect = NULL;
}

void
virtDBusConnectListFree(virtDBusConnect **connectList)
{
    if (!connectList)
        return;

    for (gint i = 0; connectList[i]; i += 1)
        virtDBusConnectFree(connectList[i]);

    g_free(connectList);
}
