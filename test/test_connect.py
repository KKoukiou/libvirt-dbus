import dbus
import libvirttest


class TestConnect(libvirttest.BaseTestClass):
    minimal_xml = '''
    <domain type="test">
      <name>foo</name>
      <memory>1024</memory>
      <os>
        <type>hvm</type>
      </os>
    </domain>
    '''

    def test_list_domains(self):
        domains = self.connect.ListDomains(0)
        assert isinstance(domains, dbus.Array)
        assert len(domains) == 1

        for path in domains:
            assert isinstance(path, dbus.ObjectPath)
            domain = self.bus.get_object('org.libvirt', path)

            # ensure the path exists by calling Introspect on it
            domain.Introspect(dbus_interface=dbus.INTROSPECTABLE_IFACE)

    def test_create(self):
        def domain_started(name, path):
            assert name == 'foo'
            assert isinstance(path, dbus.ObjectPath)
            self.loop.quit()

        self.connect.connect_to_signal('DomainStarted', domain_started)

        path = self.connect.CreateXML(self.minimal_xml, 0)
        assert isinstance(path, dbus.ObjectPath)

        self.main_loop()

    def test_define(self):
        def domain_defined(name, path):
            assert name == 'foo'
            assert isinstance(path, dbus.ObjectPath)
            self.loop.quit()

        self.connect.connect_to_signal('DomainDefined', domain_defined)

        path = self.connect.DefineXML(self.minimal_xml)
        assert isinstance(path, dbus.ObjectPath)

        self.main_loop()
