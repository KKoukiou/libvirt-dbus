#!/usr/bin/python3

import os
import subprocess
import time
import pytest
from gi.repository import GLib
from dbus.mainloop.glib import DBusGMainLoop
import dbus

ROOT = os.environ.get('abs_top_builddir', os.path.dirname(os.path.dirname(__file__)))
EXE = os.path.join(ROOT, 'src', 'libvirt-dbus')

DBusGMainLoop(set_as_default=True)


class BaseTestClass():
    """ Base test class for whole test suite
    """
    connect = None
    bus = None
    libvirt_dbus = None
    loop = False

    @pytest.fixture(autouse=True)
    def libvirt_dbus_setup(self, request):
        """Start libvirt-dbus for each test function
        """
        os.environ['LIBVIRT_DEBUG'] = '3'
        self.libvirt_dbus = subprocess.Popen([EXE])
        self.bus = dbus.SessionBus()

        for i in range(10):
            if self.bus.name_has_owner('org.libvirt'):
                break
            time.sleep(0.1)
        else:
            raise TimeoutError('error starting libvirt-dbus')

        obj = self.bus.get_object('org.libvirt', '/org/libvirt/Test')
        self.connect = dbus.Interface(obj, 'org.libvirt.Connect')

    @pytest.fixture(autouse=True)
    def libvirt_dbus_teardown(self):
        """Terminate libvirt-dbus at the teardown of each test
        """
        yield
        self.libvirt_dbus.terminate()
        self.libvirt_dbus.wait(timeout=10)

    def main_loop(self):
        """Initializes the mainloop
        """
        assert getattr(self, 'loop', False) is False

        def timeout():
            self.loop.quit()
            del self.loop
            raise TimeoutError()

        self.loop = GLib.MainLoop()
        GLib.timeout_add(1000, timeout)
        self.loop.run()
