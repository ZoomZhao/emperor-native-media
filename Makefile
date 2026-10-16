.PHONY: final-dispatch-test final-dispatch-test-undefined bink-cancel-test bink-cancel-test-undefined all game-app run-game sg3-test sg3-test-undefined data-metrics map-test map-test-undefined map-metrics map-sg3-audit map-sg3-audit-test map-raw-spans map-raw-spans-test map-reference-test map-reference-test-undefined save-state-audit save-state-audit-test pak-payload-audit pak-payload-audit-test pak-native-test pak-native-test-undefined pak-native-metrics save-native-test save-native-test-undefined save-native-metrics base-content-audit-test reference-compare-test input-test input-test-undefined input-window-test
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
map-raw-spans:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/map_raw_spans.py
map-raw-spans-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/map_raw_spans_tests.py
map-reference-test:
	/usr/bin/python3 scripts/map_reference.py test
map-reference-test-undefined:
	/usr/bin/python3 scripts/map_reference.py test --mode undefined
save-native-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py test
save-native-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py test --mode undefined
save-native-metrics:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_data.py metrics
pak-native-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py test
pak-native-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py test --mode undefined
pak-native-metrics:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_data.py metrics
pak-payload-audit:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_payload_audit.py
pak-payload-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/pak_payload_audit_tests.py
save-state-audit:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_state_audit.py
save-state-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/save_state_audit_tests.py
base-content-audit-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/base_content_audit_tests.py
reference-compare-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 scripts/reference_compare_tests.py
input-test:
	/usr/bin/python3 input_lab/run_tests.py
input-test-undefined:
	/usr/bin/python3 input_lab/run_tests.py --mode undefined
input-window-test:
	/usr/bin/python3 input_lab/run_tests.py --window

bink-cancel-test:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 game_lab/bink_cancellation/run.py
bink-cancel-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 game_lab/bink_cancellation/run.py --mode undefined

final-dispatch-test:
	PYTHONDONTWRITEBYTECODE=1 python3 input_lab/final_dispatch_tests.py

final-dispatch-test-undefined:
	PYTHONDONTWRITEBYTECODE=1 python3 input_lab/final_dispatch_tests.py --mode undefined
