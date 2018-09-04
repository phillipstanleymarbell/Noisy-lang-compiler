DIRS =\
	src/common\
	src/newton\
	src/noisy\

all: README.sloccount pre
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		$(MAKE) SYSNAME=Noisy SYSNAMELOWER=noisy\
	); \
	done

pre:
	cp config.local submodules/libflex

README.sloccount:
	sloccount src > README.sloccount

clean:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		($(MAKE) clean) \
	); \
	done; \

