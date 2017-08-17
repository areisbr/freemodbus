AM_LDFLAGS = -lpthread
AM_CFLAGS =  -I${top_srcdir}/src -pthread

LDADD = ${top_srcdir}/src/libfreemodbus.a

bin_PROGRAMS = demo
demo_SOURCES = demo.c portevent.c portother.c portserial.c porttimer.c
