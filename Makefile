.PHONY: fixtures build surface audio video metal av stream integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading
fixtures:
	/usr/bin/python3 scripts/run.py generate
build:
	/usr/bin/python3 scripts/run.py build
surface audio video metal av stream:
	/usr/bin/python3 scripts/run.py $@
integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading:
	/usr/bin/make -C media_demo $@
