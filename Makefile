DIRS =\
	src/common\
	src/newton\
#	src/noisy\

#
#	Disable Noisy build until we have updated it to use the new kCommon share constants rather than kNoisy*
#
all: README.sloccount pre newton

noisy:
	cd src/common && $(MAKE) SYSNAME=Noisy SYSNAMELOWER=noisy
	cd src/noisy && make

newton:
	cd src/common && $(MAKE) SYSNAME=Newton SYSNAMELOWER=newton
	cd src/newton && make

pre:
	cp config.local submodules/libflex
	cd submodules/libflex && make

README.sloccount:
	sloccount src > README.sloccount

clean:
	@set -e; for dir in $(DIRS); do \
	(cd $$dir; \
		($(MAKE) clean) \
	); \
	done; \

