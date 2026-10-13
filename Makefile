.PHONY: all game-app run-game sg3-test sg3-test-undefined data-metrics map-test map-test-undefined map-metrics
all: game-app
game-app:
	/bin/sh game_lab/app/build.sh
run-game: game-app
	./game_lab/build/EmperorApp.app/Contents/MacOS/EmperorApp
sg3-test:
	/usr/bin/python3 scripts/data_first.py sg3-test
sg3-test-undefined:
	/usr/bin/python3 scripts/data_first.py sg3-test --mode undefined
data-metrics:
	/usr/bin/python3 scripts/data_first.py metrics
map-test:
	/usr/bin/python3 scripts/map_data.py test
map-test-undefined:
	/usr/bin/python3 scripts/map_data.py test --mode undefined
map-metrics:
	/usr/bin/python3 scripts/map_data.py metrics
