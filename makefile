#
#             LUFA Library
#     Copyright (C) Dean Camera, 2012.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

OBJDIR       = obj
MCU          = at90usb1287
ARCH         = AVR8
BOARD        = USBKEY
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = USBModem
SRC          = $(TARGET).c                             \
               Modem_Huawei_E160x.c                    \
               Network_AUS_3_Prepaid.c                 \
               LinkManagement.c                        \
               USBManagement.c                         \
               TCPIP.c                                 \
               PPP.c                                   \
               Lib/Debug.c                             \
               $(UIP_PATH)/timer.c                     \
               $(UIP_PATH)/uip.c                       \
               $(UIP_PATH)/clock.c                     \
               $(UIP_PATH)/network.c                   \
               $(LUFA_SRC_USB)                         \
               $(LUFA_SRC_SERIAL)                      \
               $(LUFA_SRC_PLATFORM)
LUFA_PATH    = ./LUFA
UIP_PATH     = ./uIP-Contiki
CC_FLAGS    += -D USB_HOST_ONLY -D NO_STREAM_CALLBACKS -D USE_STATIC_OPTIONS="(USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
CC_FLAGS    += -D UIP_CONF_LL_802154=0 -D UIP_CONF_LL_80211=0 -D UIP_CONF_ROUTER=0 -D UIP_CONF_ICMP6=0 -D UIP_ARCH_ADD32=0 -D UIP_CONF_ICMP_DEST_UNREACH=1 -D UIP_ARCH_CHKSUM=0
LD_FLAGS     =

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
