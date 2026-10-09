.PHONY: all build verify integration-release integration-undefined gui-helper gui-regression fixtures reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-gui-long reliability-gui-reopen clean-rebuild reliability-release-metadata reliability-undefined-metadata reliability-gui-loading
all: build
build: fixtures
	/bin/sh ./build.sh
verify:
	/bin/sh ./verify.sh
integration-release:
	/usr/bin/python3 ./integration_run.py release
integration-undefined:
	/usr/bin/python3 ./integration_run.py undefined
gui-helper:
	mkdir -p build
	/usr/bin/clang++ -std=c++17 -fobjc-arc -O2 -mmacosx-version-min=15.0 -Wall -Wextra -Wpedantic -Werror gui_native.mm -framework AppKit -framework ApplicationServices -o build/gui-native
	/usr/bin/clang++ -fobjc-arc -O2 -Wall -Wextra -Wpedantic -Werror gui_permission_probe.mm -framework Foundation -framework ApplicationServices -o build/gui-permission-probe
gui-regression: gui-helper
	/usr/bin/python3 ./gui_regression.py
fixtures:
	/usr/bin/python3 ./prepare_fixtures.py
reliability-release-long:
	/usr/bin/python3 ./reliability_run.py release long
reliability-undefined-long:
	/usr/bin/python3 ./reliability_run.py undefined long
reliability-release-faults:
	/usr/bin/python3 ./reliability_run.py release faults
reliability-undefined-faults:
	/usr/bin/python3 ./reliability_run.py undefined faults
reliability-gui-long: gui-helper
	/usr/bin/python3 ./reliability_gui.py long
reliability-gui-reopen: gui-helper
	/usr/bin/python3 ./reliability_gui.py reopen
clean-rebuild:
	/usr/bin/python3 ./rebuild_clean.py
reliability-release-metadata:
	/usr/bin/python3 ./reliability_run.py release metadata
reliability-undefined-metadata:
	/usr/bin/python3 ./reliability_run.py undefined metadata
reliability-gui-loading: gui-helper
	/usr/bin/python3 ./reliability_gui.py loading
