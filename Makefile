DIRS =\
    common\
    newton\
    noisy\
	tests/tests-default\
	tests/test-step-counter\
	tests/test-activity-classifier\
	tests/test-vehicle-distance\
	tests/test-weather-balloon\
	tests/test-airplane-pressure\
	tests/test-dropped-ball\
	tests/test-gps-walking\
	tests/test-jet-engine\
	tests/test-motor-wheel-chair\
	tests/test-reactor-rod\
	tests/test-sensor-life\
	tests/test-tire-pressure\

all:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		$(MAKE)\
	); \
	done

clean:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		($(MAKE) clean) \
	); \
	done; \
