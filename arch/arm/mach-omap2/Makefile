#
# Makefile for the linux kernel.
#

# Common support
obj-y := irq.o id.o io.o sdrc.o control.o prcm.o clock.o mux.o \
		devices.o serial.o gpmc.o timer-gp.o powerdomain.o \
		clockdomain.o omapdev.o

obj-$(CONFIG_OMAP_MCBSP) += mcbsp.o

# Functions loaded to SRAM
obj-$(CONFIG_ARCH_OMAP2420)		+= sram242x.o
obj-$(CONFIG_ARCH_OMAP2430)		+= sram243x.o
obj-$(CONFIG_ARCH_OMAP3)		+= sram34xx.o

# SMS/SDRC
obj-$(CONFIG_ARCH_OMAP2)		+= sdrc2xxx.o
# obj-$(CONFIG_ARCH_OMAP3)		+= sdrc3xxx.o

# Power Management
ifeq ($(CONFIG_PM),y)
obj-y					+= pm.o
obj-$(CONFIG_ARCH_OMAP2)		+= pm24xx.o
obj-$(CONFIG_ARCH_OMAP24XX)		+= sleep24xx.o
obj-$(CONFIG_ARCH_OMAP3)		+= pm34xx.o sleep34xx.o cpuidle34xx.o
obj-$(CONFIG_PM_DEBUG)			+= pm-debug.o
endif

# SmartReflex driver
obj-$(CONFIG_OMAP_SMARTREFLEX)		+= smartreflex.o

# Clock framework
obj-$(CONFIG_ARCH_OMAP2)		+= clock24xx.o
obj-$(CONFIG_ARCH_OMAP3)		+= clock34xx.o
obj-$(CONFIG_OMAP_PM_SRF)		+=  resource34xx.o

obj-$(CONFIG_MPU_BRIDGE)		+= dspbridge.o

# DSP
obj-$(CONFIG_OMAP_MBOX_FWK)	+= mailbox_mach.o
mailbox_mach-objs		:= mailbox.o

# Debobs
obj-$(CONFIG_OMAP3_DEBOBS)	+= debobs.o

iommu-y					+= iommu2.o
iommu-$(CONFIG_ARCH_OMAP3)		+= omap3-iommu.o

obj-$(CONFIG_OMAP_IOMMU)		+= $(iommu-y)

# Specific board support
obj-$(CONFIG_MACH_OMAP_GENERIC)		+= board-generic.o
obj-$(CONFIG_MACH_OMAP_H4)		+= board-h4.o board-h4-mmc.o
obj-$(CONFIG_MACH_OMAP_2430SDP)		+= board-2430sdp.o \
					   board-2430sdp-flash.o \
					   usb-musb.o \
					   mmc-twl4030.o
obj-$(CONFIG_MACH_OMAP2EVM)		+= board-omap2evm.o \
					   mmc-twl4030.o
obj-$(CONFIG_MACH_OMAP_3430SDP)		+= board-3430sdp.o \
					   usb-musb.o \
					   mmc-twl4030.o \
					   board-3430sdp-flash.o
obj-$(CONFIG_MACH_OMAP3EVM)		+= board-omap3evm.o \
					   mmc-twl4030.o \
					   usb-musb.o \
					   board-omap3evm-flash.o \
					   twl4030-generic-scripts.o
obj-$(CONFIG_MACH_OMAP3_BEAGLE)		+= board-omap3beagle.o \
					   usb-musb.o \
					   mmc-twl4030.o \
					   twl4030-generic-scripts.o
obj-$(CONFIG_MACH_OMAP_LDP)		+= board-ldp.o \
					   usb-musb.o \
					   mmc-twl4030.o
obj-$(CONFIG_MACH_OMAP_ZOOM2)		+= board-zoom2.o \
					   usb-musb.o \
					   mmc-twl4030.o \
					   board-ldp-flash.o \
					   board-zoom2-camera.o \
					   board-connectivity.o \
					   board-zoom2-wifi.o
obj-$(CONFIG_MACH_OMAP_ZOOM3)		+= board-zoom2.o \
					   usb-musb.o \
					   mmc-twl4030.o \
					   board-ldp-flash.o \
					   board-zoom2-camera.o \
					   board-connectivity.o \
					   board-zoom2-wifi.o
obj-$(CONFIG_MACH_OMAP_3630SDP)		+= board-3630sdp.o \
					   usb-musb.o \
					   mmc-twl4030.o
obj-$(CONFIG_MACH_OMAP_APOLLON)		+= board-apollon.o \
					   board-apollon-mmc.o	\
					   board-apollon-keys.o
obj-$(CONFIG_MACH_NOKIA_N800)		+= board-n800.o board-n800-flash.o \
					   board-n800-mmc.o board-n800-bt.o \
					   board-n800-usb.o \
					   board-n800-dsp.o \
					   board-n800-camera.o
obj-$(CONFIG_MACH_NOKIA_N810)		+= board-n810.o
obj-$(CONFIG_MACH_NOKIA_RX51)		+= board-rx51.o \
					   board-n800-flash.o \
					   board-rx51-flash.o \
					   board-rx51-sdram.o \
					   board-rx51-video.o \
					   board-rx51-peripherals.o \
					   usb-musb.o \
					   mmc-twl4030.o

obj-$(CONFIG_MACH_OVERO)		+= board-overo.o \
					   usb-musb.o \
					   mmc-twl4030.o \
					   twl4030-generic-scripts.o
obj-$(CONFIG_MACH_OMAP3_PANDORA)	+= board-omap3pandora.o \
					   usb-musb.o \
					   mmc-twl4030.o

obj-$(CONFIG_MACH_ARCHOS_A28)		+= board-archos-a28.o \
					   archos-lcd-qvga28.o

obj-$(CONFIG_MACH_ARCHOS_A32)		+= board-archos-a32.o \
					   archos-lcd-byd-wqvga32.o

obj-$(CONFIG_MACH_ARCHOS_A32SD)		+= board-archos-a32sd.o \
					   archos-lcd-byd-wqvga32.o

obj-$(CONFIG_MACH_ARCHOS_A35)		+= board-archos-a35.o \
					   archos-lcd-a35.o

obj-$(CONFIG_MACH_ARCHOS_A35DM)		+= board-archos-a35dm.o \
					   archos-lcd-a35.o

obj-$(CONFIG_MACH_ARCHOS_A43)		+= board-archos-a43.o \
					   archos-compass.o \
					   archos-vibrator.o \
					   archos-nl5550.o \
					   archos-lcd-lg-fwvga43.o \
					   archos-camera-mt9d113.o 

obj-$(CONFIG_MACH_ARCHOS_A48)		+= board-archos-a48.o \
					   archos-lcd-samsung-wvga48.o \
					   archos-lcd-tpo-wvga48.o 

obj-$(CONFIG_MACH_ARCHOS_A70S)		+= board-archos-a70s.o \
					   archos-lcd-lg-wvga7.o

obj-$(CONFIG_MACH_ARCHOS_A70H)		+= board-archos-a70h.o \
					   archos-usb-ehci.o \
					   archos-usb2sata.o 

obj-$(CONFIG_MACH_ARCHOS_A101IT)	+= board-archos-a10it.o \
					   archos-lcd-boe-wsvga10.o \
					   archos-usb-touchscreen.o \
					   archos-usb-ohci.o

obj-$(CONFIG_MACH_ARCHOS)		+= archos-accel.o \
					   archos-audio.o \
					   archos-battery.o \
					   archos-camera-ov7675.o \
					   archos-die_id.o \
					   archos-fixup.o \
					   archos-keys.o \
					   archos-leds.o \
					   archos-touchscreen.o \
					   archos-wifi.o \
					   archos-usb-musb.o \
					   mmc-twl4030.o \
					   archos-usb-android.o

obj-$(CONFIG_CLOCKS_INIT) 		+= archos-clock.o

# Platform specific device init code
obj-$(CONFIG_MACH_OMAP2_TUSB6010)	+= usb-tusb6010.o

ifneq ($(CONFIG_USB_EHCI_HCD),)
	obj-y				+= usb-ehci.o
endif

ifneq ($(CONFIG_USB_OHCI_HCD),)
	obj-y				+= usb-ohci.o
endif

