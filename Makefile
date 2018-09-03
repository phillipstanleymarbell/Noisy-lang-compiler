DIRS =\
	src/common\
	src/newton\
	src/noisy\

all: README.sloccount
	cp config.local submodules/libflex; \
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		$(MAKE) SYSNAME=Noisy SYSNAMELOWER=noisy\
	); \
	done

README.sloccount:
	sloccount src > README.sloccount

clean:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		($(MAKE) clean) \
	); \
	done; \

