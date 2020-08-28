if init_script == 'systemd'
    configure_file(
        configuration: conf,
        input: 'org.libvirt-systemd.service.in',
        output: 'org.libvirt.service',
        install_dir: dbus_system_services_dir,
    )
else
    configure_file(
        configuration: conf,
        input: 'org.libvirt.service.in',
        output: 'org.libvirt.service',
        install_dir: dbus_system_services_dir,
    )
endif
configure_file(
    configuration: conf,
    input: 'org.libvirt.conf.in',
    output: 'org.libvirt.conf',
    install_dir: dbus_system_policies_dir,
)
configure_file(
    configuration: conf,
    input: 'libvirt-dbus.rules.in',
    output: 'libvirt-dbus.rules',
    install_dir: polkit_rules_dir,
)
if init_script == 'systemd'
    systemd_system_unit_dir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')
    configure_file(
        configuration: conf,
        input: 'libvirt-dbus.service.in',
        output: 'libvirt-dbus.service',
        install_dir: systemd_system_unit_dir,
    )
endif
