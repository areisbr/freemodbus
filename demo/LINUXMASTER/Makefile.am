AM_LDFLAGS = -lpthread
AM_CFLAGS =  -I${top_srcdir}/src -pthread

LDADD = ${top_srcdir}/src/libfreemodbus_m.a

bin_PROGRAMS = demo_master
demo_master_SOURCES = demo_master.c portevent.c portother.c portserial.c porttimer.c
