DIRS =\
    src/common\
    src/newton\
    src/noisy\
	src/tests/tests-default\
	src/tests/test-activity-classifier\
	src/tests/test-vehicle-distance\
	src/tests/test-weather-balloon\
	src/tests/test-step-counter\
	src/tests/test-airplane-pressure\
	src/tests/test-dropped-ball\
	src/tests/test-gps-walking\
	src/tests/test-jet-engine\
	src/tests/test-motor-wheel-chair\
	src/tests/test-reactor-rod\
	src/tests/test-sensor-life\
	src/tests/test-tire-pressure\

TESTDIRS =\
	src/tests/test-step-counter\
	src/tests/test-activity-classifier\
	src/tests/test-vehicle-distance\
	src/tests/test-weather-balloon\
	src/tests/test-airplane-pressure\
	src/tests/test-dropped-ball\
	src/tests/test-gps-walking\
	src/tests/test-jet-engine\
	src/tests/test-motor-wheel-chair\
	src/tests/test-reactor-rod\
	src/tests/test-sensor-life\
	src/tests/test-tire-pressure\

all:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		$(MAKE)\
	); \
	done

runtime:
	@set -e; for dir in $(TESTDIRS); do \
	for i in `seq 1 40`; do \
	(cd $$dir; \
		(./probe-script) \
	); \
	done; \
	(cd $$dir; \
		(python analyze_runtime.py) \
	); \
	done; \

clean:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		($(MAKE) clean) \
	); \
	done; \

