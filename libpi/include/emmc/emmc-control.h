#ifndef EMMC_CONTROL_H 
#define EMMC_CONTROL_H 

/// From Plan 9 driver 
#if 0
    /* Control0 */
	Dwidth4			= 1<<1,
	Dwidth1			= 0<<1,

	/* Control1 */
	Srstdata		= 1<<26,	/* reset data circuit */
	Srstcmd			= 1<<25,	/* reset command circuit */
	Srsthc			= 1<<24,	/* reset complete host controller */
	Datatoshift		= 16,		/* data timeout unit exponent */
	Datatomask		= 0xF0000,
	Clkfreq8shift		= 8,		/* SD clock base divider LSBs */
	Clkfreq8mask		= 0xFF00,
	Clkfreqms2shift		= 6,		/* SD clock base divider MSBs */
	Clkfreqms2mask		= 0xC0,
	Clkgendiv		= 0<<5,		/* SD clock divided */
	Clkgenprog		= 1<<5,		/* SD clock programmable */
	Clken			= 1<<2,		/* SD clock enable */
	Clkstable		= 1<<1,	
	Clkintlen		= 1<<0,		/* enable internal EMMC clocks */

	/* Cmdtm */
	Indexshift		= 24,
	Suspend			= 1<<22,
	Resume			= 2<<22,
	Abort			= 3<<22,
	Isdata			= 1<<21,
	Ixchken			= 1<<20,
	Crcchken		= 1<<19,
	Respmask		= 3<<16,
	Respnone		= 0<<16,
	Resp136			= 1<<16,
	Resp48			= 2<<16,
	Resp48busy		= 3<<16,
	Multiblock		= 1<<5,
	Host2card		= 0<<4,
	Card2host		= 1<<4,
	Autocmd12		= 1<<2,
	Autocmd23		= 2<<2,
	Blkcnten		= 1<<1,

	/* Interrupt */
	Acmderr		= 1<<24,
	Denderr		= 1<<22,
	Dcrcerr		= 1<<21,
	Dtoerr		= 1<<20,
	Cbaderr		= 1<<19,
	Cenderr		= 1<<18,
	Ccrcerr		= 1<<17,
	Ctoerr		= 1<<16,
	Err		= 1<<15,
	Cardintr	= 1<<8,		/* not in Broadcom datasheet */
	Cardinsert	= 1<<6,		/* not in Broadcom datasheet */
	Readrdy		= 1<<5,
	Writerdy	= 1<<4,
	Datadone	= 1<<1,
	Cmddone		= 1<<0,

	/* Status */
	Bufread		= 1<<11,	/* not in Broadcom datasheet */
	Bufwrite	= 1<<10,	/* not in Broadcom datasheet */
	Readtrans	= 1<<9,
	Writetrans	= 1<<8,
	Datactive	= 1<<2,
	Datinhibit	= 1<<1,
	Cmdinhibit	= 1<<0,
#endif 

/// IRQ for emmc 
#define EMMC_IRQ 62 

/// the mailbox clock id for the emmc clock 
#define EMMC_CLOCK_ID 0x01
/// write this to control1 to reset the host circuit completely 
#define EMMC_CTRL1_RESET_HOST_CIRCUIT (1 << 24)
/// write this to control1 to reset the command handling circuit 
#define EMMC_CTRL1_RESET_CMD_HANDLING_CIRCUIT (1 << 25)
/// write this to control1 to reset the data handling circuit 
#define EMMC_CTRL1_RESET_DATA_HANDLING_CIRCUIT (1 << 26)
/// Mode of clock generation: 0 = divided, 1 = programmable 
#define EMMC_CTRL1_CLK_GEN_DIV (0 << 5)
/// SD Clock enable 
#define EMMC_CTRL1_CLK_ENABLE (1 << 2) 
/// Clock enable for internal EMMC clocks for power saving 
#define EMMC_CTRL1_CLK_INTL_EN (1 << 0) 
/// SD Clock stable 
#define EMMC_CTRL1_CLK_STABLE (1 << 1)
#define EMMC_CTRL1_TOUNIT_MAX  0x000e0000
/// initialization frequency of emmc
#define EMMC_INIT_FREQUENCY 400000 
/// after initialization, the frequency will jump to 25MHz
#define EMMC_SD_FREQUENCY 25000000 
#define EMMC_SD_HIGH_FREQUENCY 50000000
/// data timeout exponent (guesswork) 
#define EMMC_DTO 14 

/// Set flag if timeout on data line 
#define EMMC_IRPT_MASK_DTO_ERR (1 << 20)
/// Set flag if card made interrupt request 
#define EMMC_IRPT_MASK_CARD_INTERRUPT (1 << 8)

/// Data transfer has finished 
#define EMMC_INTERRUPT_DATA_DONE (1 << 1)
/// Command is issued 
#define EMMC_INTERRUPT_CMD_DONE (1 << 0) 
/// An error has occured 
#define EMMC_INTERRUPT_ERR (1 << 15)
/// ready to read 
#define EMMC_INTERRUPT_READ_RDY (1 << 5)
/// ready to write 
#define EMMC_INTERRUPT_WRITE_RDY (1 << 4) 
/// card interrupt 
#define EMMC_INTERRUPT_CARD (1 << 8) 
/// various kinds of errors 
/// errors: 16:CTO, 17:CCRC, 18:CEND, 19:CBAD, 20:DTO, 21:DCRC, 22:DEND, 24:ACMD 
#define EMMC_INTERRUPT_ALL_ERROR (0x017F0000)
/// data width = 4
#define EMMC_CTRL0_HCTL_DWIDTH4 (1 << 1)
/// data width = 1 
#define EMMC_CTRL0_HCTL_DWIDTH1 (0 << 1)
/// enable high speed 
#define EMMC_CTRL0_HCTL_HS_EN (1 << 2)

/// Command line still used by previous command 
#define EMMC_STATUS_CMD_INHIBIT (1 << 0) 
/// Data lines still used by previous data transfer 
#define EMMC_STATUS_DATA_INHIBIT (1 << 1) 
/// Data available for read 
#define EMMC_STATUS_READ_AVAILABLE (1 << 9) 
// 0x00000800 ? 
// (1 << 9) ? 

#define EMMC_BLKCNT_MASK (0xFFFF0000)
#define EMMC_BLKCNT_SHIFT 16 

#define EMMC_DMA_CHANNEL 4 
#define EMMC_DMA_DEVICE 11 

#define EMMC_SCR_SD_BUS_WIDTH_4  0x00000400
#define EMMC_SCR_SUPP_SET_BLKCNT 0x02000000
#define EMMC_SCR_SUPP_CCS        0x00000001

#endif // ! EMMC_CONTROL_H 

















