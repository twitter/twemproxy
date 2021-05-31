
dist_check_SCRIPTS = check_server_fail_between_requests.py check_server_fail_during_requests.py check_server_fail_and_recover.py check_config_reload.py check_config_reload_after_failure.py

EXTRA_DIST = test_helper.py

TESTS = $(dist_check_SCRIPTS)

TESTS_ENVIRONMENT =	\
	PYTHONDONTWRITEBYTECODE=avoid_pyc_files	\
	NUTCRACKER_PROGRAM="$(top_builddir)/src/nutcracker"

clean-local:
	rm -f *.pyc
