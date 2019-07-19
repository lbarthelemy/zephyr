/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <arch/cpu.h>

#define LPUART0SRC_MCGPCLK (1)
#define SIM_OSC32KSEL_OSC32KCLK_CLK                       0U  /*!< OSC32KSEL select: OSC32KCLK clock */

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*
 * KL27Z Flash configuration fields
 * These 16 bytes, which must be loaded to address 0x400, include default
 * protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 *
 * The structure is:
 * -Backdoor Comparison Key for unsecuring the MCU - 8 bytes
 * -Program flash protection bytes, 4 bytes, written to FPROT0-3
 * -Flash security byte, 1 byte, written to FSEC
 * -Flash nonvolatile option byte, 1 byte, written to FOPT
 * -Reserved, 1 byte, (Data flash protection byte for FlexNVM)
 * -Reserved, 1 byte, (EEPROM protection byte for FlexNVM)
 *
 */
u8_t __kinetis_flash_config_section __kinetis_flash_config[] = {
	/* Backdoor Comparison Key (unused) */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	/* Program flash protection; 1 bit/region - 0=protected, 1=unprotected
	 */
	0xFF, 0xFF, 0xFF, 0xFF,
	/*
	 * Flash security: Backdoor key disabled, Mass erase enabled,
	 *                 Factory access enabled, MCU is unsecure
	 */
	0xFE,
	/* Flash nonvolatile option: NMI enabled, EzPort enabled, Normal boot */
	0xFF,
	/* Reserved for FlexNVM feature (unsupported by this MCU) */
	0xFF, 0xFF};

/*
 * How to setup clock using clock driver functions:
 *
 * 1. CLOCK_SetSimSafeDivs, to make sure core clock, bus clock, flexbus clock
 *    and flash clock are in allowed range during clock mode switch.
 *
 * 2. Call CLOCK_Osc0Init to setup OSC clock, if it is used in target mode.
 *
 * 3. Call CLOCK_SetMcgliteConfig to set MCG_Lite configuration.
 *
 * 4. Call CLOCK_SetSimConfig to set the clock configuration in SIM.
 */
static ALWAYS_INLINE void clkInit(void)
{
	/*******************************************************************************
	 * Variables for BOARD_BootClockRUN configuration
	 ******************************************************************************/
	const mcglite_config_t mcgliteConfig_BOARD_BootClockRUN =
	{
		.outSrc = kMCGLITE_ClkSrcHirc,            /* MCGOUTCLK source is HIRC */
		.irclkEnableMode = kMCGLITE_IrclkEnable,  /* MCGIRCLK enabled, MCGIRCLK disabled in STOP mode */
		.ircs = kMCGLITE_Lirc8M,                  /* Slow internal reference (LIRC) 8 MHz clock selected */
		.fcrdiv = kMCGLITE_LircDivBy1,            /* Low-frequency Internal Reference Clock Divider: divided by 1 */
		.lircDiv2 = kMCGLITE_LircDivBy1,          /* Second Low-frequency Internal Reference Clock Divider: divided by 1 */
		.hircEnableInNotHircMode = true,          /* HIRC source is enabled */
	};
	const sim_clock_config_t simConfig_BOARD_BootClockRUN =
	{
		.er32kSrc = SIM_OSC32KSEL_OSC32KCLK_CLK,  /* OSC32KSEL select: OSC32KCLK clock */
		.clkdiv1 = 0x10000U,                      /* SIM_CLKDIV1 - OUTDIV1: /1, OUTDIV4: /2 */
	};
//	const osc_config_t oscConfig_BOARD_BootClockRUN =
//	{
//		.freq = 0U,                               /* Oscillator frequency: 0Hz */
//		.capLoad = (kOSC_Cap4P | kOSC_Cap8P),     /* Oscillator capacity load: 12pF */
//		.workMode = kOSC_ModeOscLowPower,         /* Oscillator low power */
//		.oscerConfig =
//		{
//			.enableMode = kOSC_ErClkEnable,   /* Enable external reference clock, disable external reference clock in STOP mode */
//		}
//	};

     /* Set the system clock dividers in SIM to safe value. */
    CLOCK_SetSimSafeDivs();
    /* Set MCG to HIRC mode. */
    CLOCK_SetMcgliteConfig(&mcgliteConfig_BOARD_BootClockRUN);
    /* Set the clock configuration in SIM module. */
    CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
    /* Set SystemCoreClock variable. */
    //SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;
#ifdef CONFIG_UART_MCUX_LPUART_0
    CLOCK_SetLpuart0Clock(LPUART0SRC_MCGPCLK);
#endif

}

static int kl2x_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Disable the watchdog */
	SIM->COPC = 0;

	/* Initialize system clock to 48 MHz */
	clkInit();
	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(kl2x_init, PRE_KERNEL_1, 0);
