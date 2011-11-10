# -----------------------------------------------------------------------------
#
#   FILE: Makefile
#
#       Build instructions for unifimib and unifidl.
#
#  HISTORY:
#   22-Nov-05  MEDP Use $CROSS_COMPILE prefix for tools.
#   05-Oct-05  MEDP Build unifidl too.
#   02-Sep-05  MEDP Creation
# -----------------------------------------------------------------------------

CC := $(CROSS_COMPILE)gcc

CFLAGS := -I../../common -I../driver -I../../lib_sme/common -I../../lib_synergy_framework/linux -Wall -g -O2
LDFLAGS := -g

UM_OBJS := unifi_manager.o mibquery.o sme_drv.o

UF_CFG_OBJS := unifi_config_lib.o unifi_config.o

UF_PUTEST_OBJS := unifi_putest_lib.o unifi_putest.o

GSMIB_OBJS := mibquery.o unifi_mib.o

UF_CORED_OBJS := unifi_coredump.o

UF_XBV_OBJS := unifi_xbv.o xbv_user.o

SBINDIR := /usr/sbin

PROGS := unifi_manager unifi_config
SCRIPTS := unififw

# Local tools not for distribution
TOOLS := genmacmib prmib unifi_qos unifi_mib unifi_putest unifi_coredump unifi_xbv

all: $(PROGS) $(TOOLS)

unifi_manager: $(UM_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(UM_OBJS)


genmacmib: genmacmib.o
	$(CC) $(LDFLAGS) -o $@ genmacmib.o

prmib: prmib.o
	$(CC) $(LDFLAGS) -o $@ prmib.o

unifi_qos: unifi_qos.o
	$(CC) $(LDFLAGS) -o $@ unifi_qos.o

unifi_config: $(UF_CFG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(UF_CFG_OBJS)

unifi_putest: $(UF_PUTEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(UF_PUTEST_OBJS)

unifi_mib:  $(GSMIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(GSMIB_OBJS)

unifi_coredump:  $(UF_CORED_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(UF_CORED_OBJS)

# Special build of XBV parser for standalone test
xbv_user.o:	../../lib_hip/xbv.c
	$(CC) -c $(CFLAGS) -DCSR_WIFI_XBV_TEST -I. $(LDFLAGS) -o $@ ../../lib_hip/xbv.c

unifi_xbv.o:  unifi_xbv.c unifi_xbv.h
	$(CC) -c $(CFLAGS) -DCSR_WIFI_XBV_TEST -o $@ $<

unifi_xbv:  $(UF_XBV_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(UF_XBV_OBJS)

install: $(PROGS)
	for p in $(PROGS) $(SCRIPTS); do install -D $$p $(DESTDIR)$(SBINDIR)/$$p; done

clean:
	rm -f *.o $(PROGS) genmacmib prmib unifi_qos unifi_mib unifi_config unifi_putest unifi_coredump unifi_xbv
