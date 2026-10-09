.PHONY: fixtures build surface audio video metal av stream game game-app run-game integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading
fixtures:
	/usr/bin/python3 scripts/run.py generate
build:
	/usr/bin/python3 scripts/run.py build
surface audio video metal av stream game:
	/usr/bin/python3 scripts/run.py $@
game-app:
	/bin/sh game_lab/app/build.sh
run-game: game-app
	open game_lab/build/EmperorApp.app
integration-release integration-undefined reliability-release-long reliability-undefined-long reliability-release-faults reliability-undefined-faults reliability-release-metadata reliability-undefined-metadata clean-rebuild gui-helper gui-regression reliability-gui-long reliability-gui-reopen reliability-gui-loading:
	/usr/bin/make -C media_demo $@
