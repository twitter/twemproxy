
dist_check_SCRIPTS = check_config_reload.py

EXTRA_DIST = test_helper.py

TESTS = $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT =	\
	PYTHONDONTWRITEBYTECODE=avoid_pyc_files	\
	NUTCRACKER_PROGRAM="$(top_builddir)/src/nutcracker"

clean-local:
	rm -f *.pyc
