.PHONY: fixtures build surface audio video metal av stream game game-boundaries runtime-boundaries game-app run-game integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading
fixtures:
	/usr/bin/python3 scripts/run.py generate
build:
	/usr/bin/python3 scripts/run.py build
surface audio video metal av stream game:
	/usr/bin/python3 scripts/run.py $@
runtime-boundaries:
	/bin/bash runtime_core/run_boundaries.sh
game-boundaries:
	/bin/bash game_lab/run_boundaries.sh
game-app:
	/bin/sh game_lab/app/build.sh
run-game: game-app
	open game_lab/build/EmperorApp.app --args --data-root "$(CURDIR)/GameData"
integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading:
	/usr/bin/make -C media_demo $@

.PHONY: game-app-integration
game-app-integration:
	/bin/bash game_lab/run_app_integration.sh
