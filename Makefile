TOPDIR ?= ../../..
include $(TOPDIR)/.config

$(NRF24L01_SUPPORT)_SRC += hardware/nrf24l01/driver/nrf24l01.c
##############################################################################
# generic fluff
include $(TOPDIR)/scripts/rules.mk
