#ifndef _HX8352_H
#define _HX8352_H

// HX8352-A registers for lcd interface
#define HX8352_ID 0x74
#define HX8552_RS 0x02		// set for a command, unset for index register access
#define HX8552_READ 0x01
#define HX8552_WRITE 0x00


#define  HX8352_HW			0x00
#define  HX8352_DISPMODE		0x01
#define  HX8352_COLSTART		0x02
#define  HX8352_COLSTART1		0x03
#define  HX8352_COLSTOP			0x04
#define  HX8352_COLSTOP1		0x05
#define  HX8352_ROWSTART		0x06
#define  HX8352_ROWSTART1		0x07
#define  HX8352_ROWSTOP			0x08
#define  HX8352_ROWSTOP1		0x09
#define  HX8352_PA_ROWSTART		0x0a
#define  HX8352_PA_ROWSTART1		0x0b
#define  HX8352_PA_ROWEND		0x0c
#define  HX8352_PA_ROWEND1		0x0d
#define  HX8352_VSCROLLTOP		0x0e
#define  HX8352_VSCROLLTOP1		0x0f
#define  HX8352_VSCROLL_H		0x10
#define  HX8352_VSCROLL_H1		0x11
#define  HX8352_VSCROLL_B		0x12
#define  HX8352_VSCROLL_B1		0x13
#define  HX8352_VSCROLL_ADD		0x14
#define  HX8352_VSCROLL_ADD1		0x15

#define  HX8352_MEM_CTRL		0x16
#define  HX8352_OSC1_CTRL		0x17
#define  HX8352_OSC2_CTRL		0x18
#define  HX8352_PW1_CTRL		0x19
#define  HX8352_PW2_CTRL		0x1a
#define  HX8352_PW3_CTRL		0x1b
#define  HX8352_PW4_CTRL		0x1c
#define  HX8352_PW5_CTRL		0x1d
#define  HX8352_PW6_CTRL		0x1e
#define  HX8352_VCOM			0x1f
#define  HX8352_READ_DATA		0x22
#define  HX8352_DISP_CTRL1		0x23
#define  HX8352_DISP_CTRL2		0x24
#define  HX8352_DISP_CTRL3		0x25
#define  HX8352_DISP_CTRL4		0x26
#define  HX8352_DISP_CTRL5		0x27
#define  HX8352_DISP_CTRL6		0x28
#define  HX8352_DISP_CTRL7		0x29
#define  HX8352_DISP_CTRL8		0x2a
#define  HX8352_CYC_CTRL1		0x2b
#define  HX8352_CYC_CTRL2		0x2c
#define  HX8352_CYC_CTRL3		0x2d
#define  HX8352_CYC_CTRL4		0x2d
#define  HX8352_CYC_CTRL5		0x2e
#define  HX8352_CYC_CTRL6		0x2f
#define  HX8352_CYC_CTRL7		0x30
#define  HX8352_CYC_CTRL8		0x31
#define  HX8352_CYC_CTRL9		0x32
#define  HX8352_CYC_CTRL10		0x34
#define  HX8352_CYC_CTRL11		0x35
#define  HX8352_CYC_CTRL12		0x36
#define  HX8352_CYC_CTRL13		0x37
#define  HX8352_CYC_CTRL14		0x38
#define  HX8352_CYC_CTRL15		0x39

#define  HX8352_IF_CTRL1		0x3a
#define  HX8352_SRC_CTRL1		0x3c
#define  HX8352_SRC_CTRL2		0x3d

#define  HX8352_GAMMA1			0x3e
#define  HX8352_GAMMA2			0x3f
#define  HX8352_GAMMA3			0x40
#define  HX8352_GAMMA4			0x41

#define  HX8352_GAMMA5			0x42
#define  HX8352_GAMMA6			0x43
#define  HX8352_GAMMA7			0x44
#define  HX8352_GAMMA8			0x45
#define  HX8352_GAMMA9			0x46
#define  HX8352_GAMMA10			0x47
#define  HX8352_GAMMA11			0x48
#define  HX8352_GAMMA12			0x49

#define  HX8352_PANEL_CTRL		0x55
#define  HX8352_OTP1			0x56
#define  HX8352_OTP2			0x57
#define  HX8352_OTP3			0x58
#define  HX8352_OTP4			0x59
#define  HX8352_IP_CTRL			0x5a
#define  HX8352_DGCLUT			0x5c

#define  HX8352_CABCPWM_CTRL1		0x6b
#define  HX8352_CABCPWM_CTRL2		0x6c
#define  HX8352_CABC1			0x6f
#define  HX8352_CABC2			0x70
#define  HX8352_CABC3			0x71
#define  HX8352_CABC4			0x72
#define  HX8352_CABC5			0x73
#define  HX8352_CABC6			0x74
#define  HX8352_CABC7			0x75
#define  HX8352_CABC8			0x76
#define  HX8352_CABC9			0x77


#define  HX8352_TEST			0x83
#define  HX8352_VDD_CTRL		0x85
#define  HX8352_PW_DRIVE		0x8a

#define  HX8352_GAMMAR1			0x8b
#define  HX8352_GAMMAR2			0x8c

#define  HX8352_SYNC			0x91
#define  HX8352_PWM_CTRL1		0x95
#define  HX8352_PWM_CTRL2		0x96
#define  HX8352_PWM_CTRL3		0x97





#endif /* _HX8352_H */
