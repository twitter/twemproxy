#!/bin/bash -xeu
# A simple utility script to run tests with extremely verbose output.

if [[ $# == 0 ]]; then
	echo "Usage: $0 test_a [test_b, ...]" 1>&2
	exit 1
fi

# Print test logging to stderr
export T_LOGFILE=-

python3 -m nose -v --nologcapture --nocapture "$@"
