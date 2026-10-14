.PHONY: all game-app run-game sg3-test sg3-test-undefined data-metrics map-test map-test-undefined map-metrics map-sg3-audit map-sg3-audit-test reference-compare-test input-test input-test-undefined input-window-test
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
map-sg3-audit:
	/usr/bin/python3 scripts/map_sg3_audit.py
map-sg3-audit-test:
	/usr/bin/python3 scripts/map_sg3_audit_tests.py
reference-compare-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/reference_compare_tests.py
input-test:
	/usr/bin/python3 input_lab/run_tests.py
input-test-undefined:
	/usr/bin/python3 input_lab/run_tests.py --mode undefined
input-window-test:
	/usr/bin/python3 input_lab/run_tests.py --window
