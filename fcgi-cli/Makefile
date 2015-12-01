PROGRAM  = fcgi-cli
TARGETS  = fcgi-cli
DEP      = .depend
SRCS     = main.c fcgi.c
OBJS     = $(addsuffix .o, $(basename $(SRCS)))

CCFLAGS = -L. -I. -Wall -g
CCLIBS  = -luv
CC      = gcc

all: dep $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o $@ $(CCLIBS)

.c.o:
	$(CC) $(CCFLAGS) -c $<

dep:
ifeq ($(DEP),$(wildcard $(DEP)))
-include $(DEP)
else
	$(CC) -MM $(CCFLAGS) $(SRCS) > $(DEP)
endif

clean:
	rm -f $(PROGRAM) $(OBJS) $(DEP)
