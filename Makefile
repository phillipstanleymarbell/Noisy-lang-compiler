DIRS =\
    noisy\
    newton\
    common\
	include\

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

