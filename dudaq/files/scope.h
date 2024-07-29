/// @file scope.h
/// @brief Header file for GP300 scope
/// @author C. Timmermans, Nikhef/RU

/*
 * Header file for GP300 scope - DMA version
 *
 * C. Timmermans/N. Cucu Laurenciu
 * c.timmermans@science.ru.nl
 *
 * NOT backward compatible with older versions!
 *>
 */

#include <stdint.h>

/*********************************************************************/
/*                       define mmap locations                       */
/*********************************************************************/
#define DDR_BASE_ADDR               0x20000000      /* base address DDR */
#define CDMA_BASE_ADDR              0xA0000000      /* base address CDMA regs configuration (FPGA) */
#define SG_BASE_ADDR                0xA0008000      /* base address scatter-gather configuration (FPGA) */
#define RAM1_BASE_ADDR              0xC0000000      /* base address RAM1 - event header (FPGA) */
#define RAM2_BASE_ADDR              0xC2000000      /* base address RAM2 - CH1 RAM pre  (preTrigger+Overlap) data samples (FPGA) */
#define RAM3_BASE_ADDR              0xC4000000      /* base address RAM3 - CH2 RAM pre  (preTrigger+Overlap) data samples (FPGA) */
#define RAM4_BASE_ADDR              0xC6000000      /* base address RAM4 - CH3 RAM pre  (preTrigger+Overlap) data samples (FPGA) */
#define RAM5_BASE_ADDR              0xC8000000      /* base address RAM5 - CH1 RAM post (postTrigger) data samples (FPGA) */
#define RAM6_BASE_ADDR              0xCA000000      /* base address RAM6 - CH2 RAM post (postTrigger) data samples (FPGA) */
#define RAM7_BASE_ADDR              0xCC000000      /* base address RAM7 - CH3 RAM post (postTrigger) data samples (FPGA) */
#define RAM8_BASE_ADDR              0xCE000000      /* base address RAM8 - PPS (FPGA) */

#define TDAQDUregs_BASE_ADDR        0x80000000      /* base address DU registers (FPGA) */
/*********************************************************************/
/*                       define mmap size                            */
/*********************************************************************/
#define NUM_OF_DESCRIPTORS          8              /* number of transfer descriptors (1 for header + 4 channels * 2 for data) */
#define DESCRIPTOR_SIZE             64             /* transfer descriptors are aligned on 16 32-bit word alignment. => 16*4=64B per transfer descriptor */

#define DDR_MAP_SIZE                0x20000000     /* 512MB */
#define RAM_DATA_MAP_SIZE           16384          /* 16KB for all data RAMs (RAM2 - RAM7) */
#define RAM_HEADER_MAP_SIZE         1024           /* for the event header RAM (RAM1) and PPS RAM (RAM8) */
#define CDMA_MAP_SIZE               128            /* 11 32-bit registers, memory alignment on 128-byte boundaries */
#define SG_MAP_SIZE                 (DESCRIPTOR_SIZE*NUM_OF_DESCRIPTORS) /* 576B but will allocate the minimum page size (4096B)? */

/*********************************************************************/
/*             define CDMA registers locations & configs             */
/*********************************************************************/
// -- address offsets for CDMA registers (DMA in Scatter-Gather Mode)
#define CDMA_CR                     0x00000000      /* Control register */
#define CDMA_SR                     0x00000004      /* Status register */
#define CDMA_CURDESC_PNTR           0x00000008      /* Current descriptor pointer */
#define CDMA_CURDESC_PNTR_MSB       0x0000000C      /* Current descriptor pointer MSB */
#define CDMA_TAILDESC_PNTR          0x00000010      /* Tail descriptor pointer */
#define CDMA_TAILDESC_PNTR_MSB      0x00000014      /* Tail descriptor pointer MSB */

// -- address offsets for scatter-gather transfer descriptors registers
#define SG_NXTDESC_PNTR             0x00000000      /* Next descriptor pointer */
#define SG_NXTDESC_PNTR_MSB         0x00000004      /* Next descriptor pointer MSB */
#define SG_SA                       0x00000008      /* Source address */
#define SG_SA_MSB                   0x0000000C      /* Source address MSB */
#define SG_DA                       0x00000010      /* Destination address */
#define SG_DA_MSB                   0x00000014      /* Destination address MSB */
#define SG_CONTROL                  0x00000018      /* Transfer control */
#define SG_STATUS                   0x0000001C      /* Transfer status */

// -- configuration for CDMA_CR register
#define IRQ_MASK_AND_SG             0x02027008     /* IRQDelay=0x02 (125*SGclock*IRQDelay); IRQThreshold=0x02; IOC_IrqEn=1; SGMmode=1 */
#define IRQ_MASK_AND_SG_evt       0x02077008   /* IRQDelay=0x02 (125*SGclock*IRQDelay); IRQThreshold=0x07; IOC_IrqEn=1; SGMmode=1 */
#define IRQ_MASK_AND_SG_pps       0x02017008   /* IRQDelay=0x02 (125*SGclock*IRQDelay); IRQThreshold=0x01; IOC_IrqEn=1; SGMmode=1 */
#define IRQ_MASK_AND_SG_evtpps    0x02087008   /* IRQDelay=0x02 (125*SGclock*IRQDelay); IRQThreshold=0x08; IOC_IrqEn=1; SGMmode=1 */


/*----------------------------------------------------------------------*/
#define SAMPLING_FREQ 500 //!< 500 MHz scope
#define ADC_RESOLUTION 14

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Maxima / minima */

/* Maximum ADC size = 4 channels * max_samples/ch * 2 bytes/sample */
/* Maximum event size = header + ADC data + message end       */
#define DATA_MAX_SAMP   8192                       //!< Maximal trace length (samples)

#define MAX_READOUT     (146*2 + 3*DATA_MAX_SAMP) //!< Maximal raw event size

/*----------------------------------------------------------------------*/
#define TDAQ_BASE             0x80000000
/* Register Definitions*/
#define Reg_HVL            0x0000
#define Reg_HTL            0x0004
#define Reg_HTLH_GPS       0x0008
#define Reg_HTLL_GPS       0x000C
#define Reg_MNG            0x004C
#define Reg_DMA            0x0050
#define Reg_AGC12          0x0054
#define Reg_AGC34          0x0058
#define Reg_RW             0x0010
#define Reg_RS             0x0014
#define Reg_TS             0x0018
#define Reg_CH1SNTH        0x001C
#define Reg_CH1TP          0x0020
#define Reg_CH2SNTH        0x0028
#define Reg_CH2TP          0x002C
#define Reg_CH3SNTH        0x0034
#define Reg_CH3TP          0x0038
#define Reg_BSC12          0x005C
#define Reg_BSC34          0x0060
#define Reg_CH1NF1P1       0x0064
#define Reg_CH1NF2P1       0x007C
#define Reg_CH1NF3P1       0x0094
#define Reg_CH1NF4P1       0x00AC
#define Reg_CH2NF1P1       0x00C4
#define Reg_CH2NF2P1       0x00DC
#define Reg_CH2NF3P1       0x00F4
#define Reg_CH2NF4P1       0x010C
#define Reg_CH3NF1P1       0x0124
#define Reg_CH3NF2P1       0x013C
#define Reg_CH3NF3P1       0x0154
#define Reg_CH3NF4P1       0x016C
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* PPS definition */
#define MAGIC_PPS     0xFACE
#define WCNT_PPS        22 // length in 32 bit words!
#define PPS_LENGTH      0
#define PPS_ID          1
#define PPS_CTP         2
#define PPS_WEEKTIME    3
#define PPS_WEEKOFFSET  4
#define PPS_SECMINHOUR  5
#define PPS_DAYMONTH    6
#define PPS_YEAR        7
#define PPS_GPSMODE     8
#define PPS_GPSSTATUS   9
#define PPS_OFFSET      10
#define PPS_TEMPERATURE 11
#define PPS_LATITUDE    12
#define PPS_LONGITUDE   14
#define PPS_ALTITUDE    16
#define PPS_ATM_TP      18
#define PPS_HM_AX       19
#define PPS_AY_AZ       20
#define PPS_BATTERY     21

/*----------------------------------------------------------------------*/
/* Event definition */
#define MAGIC_EVT         0xADC0
#define EVT_HDR_LENGTH    146
#define EVT_LENGTH        0 // nr of int32 words
#define EVT_VERSION       1
#define EVT_STATION_ID    2
#define EVT_HARDWARE_ID   3
#define EVT_EVT_ID        4
#define EVT_CTP           5
#define EVT_CTD           6
#define EVT_ADCINFO       7
#define EVT_SECOND        8
#define EVT_NANOSEC       9
#define EVT_TRIGGER_POS   10
#define EVT_TRIGGER_STAT  11
#define EVT_STATISTICS    12
#define EVT_PPS_ID        13
#define EVT_SPARE1        14
#define EVT_SPARE2        15
#define EVT_SPARE3        16
#define EVT_ATM_TP        17
#define EVT_HM_AX         18
#define EVT_AY_AZ         19
#define EVT_BATTERY       20
#define EVT_WEEKTIME      21
#define EVT_WEEKOFFSET    22
#define EVT_SECMINHOUR    23
#define EVT_DAYMONTH      24
#define EVT_YEAR          25
#define EVT_GPSMODE       26
#define EVT_GPSSTATUS     27
#define EVT_OFFSET        28
#define EVT_TEMPERATURE   29
#define EVT_LATITUDE      30
#define EVT_LONGITUDE     32
#define EVT_ALTITUDE      34
#define EVT_TRACELENGTH   36
#define EVT_INP_SELECT    37
#define EVT_TRIG_SELECT   38
#define EVT_THRES_C1      39
#define EVT_THRES_C2      40
#define EVT_THRES_C3      41
#define EVT_TRIG_C1       43
#define EVT_TRIG_C2       45
#define EVT_TRIG_C3       47
#define EVT_GAIN_AB       51
#define EVT_GAIN_CD       52
#define EVT_BASELINE_12   53
#define EVT_BASELINE_3    54
#define EVT_NOTCH_C1_F1   63
#define EVT_NOTCH_C1_F2   68
#define EVT_NOTCH_C1_F3   73
#define EVT_NOTCH_C1_F4   78
#define EVT_NOTCH_C2_F1   83
#define EVT_NOTCH_C2_F2   88
#define EVT_NOTCH_C2_F3   93
#define EVT_NOTCH_C2_F4   98
#define EVT_NOTCH_C3_F1   103
#define EVT_NOTCH_C3_F2   108
#define EVT_NOTCH_C3_F3   113
#define EVT_NOTCH_C3_F4   118
#define EVT_TOT_SAMPLEP   143
#define EVT_CH12_SAMPLEP  144
#define EVT_CH3_SAMPLEP   145
#define EVT_START_ADC     146

/*----------------------------------------------------------------------*/

/*
  buffer definitions for the scope readout process.
 */

#define BUFSIZE 80000            //!< store up to 80000 event pointers in circular buffer

#define GPSSIZE 35              //!< buffer upto 35 GPS pointers in circular buffer

// next: what did we read from the scope?

#define SCOPE_EVENT 2          //!< return code for reading an event
#define SCOPE_GPS   3          //!< return code for reading a PPS message

#define TRIGGER_T3_MINBIAS 0x1000
#define TRIGGER_T3_RANDOM  0x8000


typedef struct
{
  uint32_t ts_seconds;
  uint32_t ts_nanoseconds;
  uint16_t event_nr;
  uint16_t trigmask;
}TS_DATA; //timestamps

typedef struct
{
  uint32_t ts_seconds;      //!< time marker in GPS sec
  uint32_t data[WCNT_PPS];  //! all data read in PPS
} GPS_DATA;


// the routines

void scope_raw_write(uint32_t reg_addr, uint32_t value);
uint32_t scope_raw_read(uint32_t reg_addr);
void scope_flush();
int scope_open();
void scope_close();
void scope_reset();
void scope_start_run();
void scope_stop_run();
void scope_reboot();
void scope_initialize();
void scope_create_memory();
int scope_read_event(uint32_t index);
int scope_read_pps(uint32_t index);
int scope_read();
int scope_calc_t3nsec();
//

