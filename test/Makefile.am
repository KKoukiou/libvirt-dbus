test_helpers = \
	libvirttest.py \
	conftest.py

test_programs = \
	test_connect.py \
	test_domain.py

EXTRA_DIST = \
	$(test_helpers) \
	$(test_programs) \
	travis-run

TEST_PATH=./

lint:
	flake8 --exclude=.tox --ignore=E501

test:
	py.test --verbose --color=yes $(TEST_PATH)

check: lint test

TESTS_ENVIRONMENT = \
	abs_top_builddir=$(abs_top_builddir) \
	VIRT_DBUS_INTERFACES_DIR=$(abs_top_srcdir)/data
