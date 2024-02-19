/// @file scope.c
/// @brief routines interfacing to the fpga
/// @author C. Timmermans, N. Cucu Laurenciu Nikhef/RU

/************************************
 File: scope.c
 Author: C. Timmermans, N. Cucu Laurenciu Nikhef/RU
 
 
 Buffers:
 eventbuf: holds the events read-out from the digitizer
 evread: index of location of next event read-out
 evgps: index of location of next GPS data read out
 ************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>
#include<errno.h>

#include "ad_shm.h"
#include "dudaq.h"
#include "scope.h"
#include "tflite_inference.h"   // TFLT_xxx()
#include "ring_buffer_eval.h"  // RBE_xxx()
#include "func_eval_evt.h" // FEEV_xxx()

/* EXTERNAL */
S_RingBufferEval* Gp_rbuftrig[2]

int32_t dev = 0;
void* axi_ptr;
uint32_t page_offset;

extern shm_struct shm_gps;
int hw_id = 0;
uint32_t last_sec;
uint32_t prev_ppsid = 0;
extern int station_id;

uint32_t* vadd_ram1, * vadd_ram2, * vadd_ram3;
uint32_t* vadd_ram4, * vadd_ram5, * vadd_ram6;
uint32_t* vadd_ram7, * vadd_ram8;
uint32_t* vadd_psddr, * vadd_cdma, * vadd_sg;
unsigned int page_size;
volatile uint8_t resetBit;
volatile uint8_t idleBit;
volatile uint8_t IOC_Irq, errorIrq;
volatile uint8_t ppsReady, evtReady;
volatile uint8_t SGDecErr, SGSlvErr, SGIntErr;
volatile uint8_t DMADecErr, DMASlvErr, DMAIntErr;

uint32_t addr1, addr2, addr3;
uint32_t addr4, addr5, addr6;
uint32_t addr7, addr8;
uint32_t ddrOffset = 0, ddrPrevOffset = 0;
uint32_t eventLength = 0, ppsLength;
uint32_t Overlap, preOverlap, postOverlap;

void short_wait()
{
   usleep(1);
}

unsigned int dma_wr_reg(unsigned int* dma_virtual_address, int offset, unsigned int value)
{
   dma_virtual_address[offset >> 2] = value;
   return (0);
}

unsigned int dma_rd_reg(unsigned int* dma_virtual_address, int offset)
{
   return dma_virtual_address[offset >> 2];
}

unsigned int dma_wr_bit(
                        unsigned int* dma_virtual_address,
                        int offset,
                        uint8_t bitNo,
                        uint8_t bitVal)
{
   int32_t mask = 1 << bitNo;
   int32_t current_value = dma_virtual_address[offset >> 2];
   int32_t new_value = ((current_value & ~mask) | (bitVal << bitNo));
   dma_virtual_address[offset >> 2] = new_value;
   return (0);
}

unsigned int dma_rd_bit(unsigned int* dma_virtual_address, int offset, uint8_t bitNo)
{
   int32_t current_value = dma_virtual_address[offset >> 2];
   return (current_value & (1 << bitNo)) >> bitNo;
}

void DUreg_write(uint32_t reg_addr, uint32_t value)
{
   *((unsigned int*) (axi_ptr + page_offset + reg_addr)) = value;
}

uint32_t DUreg_read(uint32_t reg_addr)
{
   return *((unsigned int*) (axi_ptr + page_offset + reg_addr));
}

unsigned int DUreg_rd_bit(uint32_t reg_addr, uint8_t bitNo)
{
   uint32_t regValue = DUreg_read(reg_addr);
   return (regValue & (1 << bitNo)) >> bitNo;
}

unsigned int DUreg_rd_excerpt(uint32_t reg_addr, uint8_t bitNoStart, uint8_t bitNoStop)
{
   uint32_t regValue = DUreg_read(reg_addr);
   if (bitNoStart > bitNoStop)
   {
      printf("ERROR: bitNoStart sjould be smaller than bitNoStop");
      exit(0);
   }
   unsigned int mask = 0;
   for (unsigned i = bitNoStart; i <= bitNoStop; i++)
      mask |= 1 << i;
   return (mask & regValue) >> bitNoStart;
}

unsigned int print_CDMA_regs(unsigned int* dma_virtual_address)
{
   printf("    CDMACR        :0x%08x\n", dma_rd_reg(dma_virtual_address, CDMA_CR));
   printf("    CDMASR        :0x%08x\n", dma_rd_reg(dma_virtual_address, CDMA_SR));
   printf("    CURDESC_PNTR  :0x%08x\n", dma_rd_reg(dma_virtual_address, CDMA_CURDESC_PNTR));
   printf("    TAILDESC_PNTR :0x%08x\n", dma_rd_reg(dma_virtual_address, CDMA_TAILDESC_PNTR));
   return (0);
}

// -- for DMA in Scatter-Gather Mode, print the buffer descriptors
unsigned int print_CDMA_SG_descriptors(unsigned int* dma_SG_virtual_address, int offset)
{
   printf("    Transfer descriptors at base address: 0x%08x\n", SG_BASE_ADDR + offset);
   printf("    NXTDESC_PNTR :0x%08x\n",
          dma_rd_reg(dma_SG_virtual_address, offset + SG_NXTDESC_PNTR));
   printf("    SA           :0x%08x\n", dma_rd_reg(dma_SG_virtual_address, offset + SG_SA));
   printf("    DA           :0x%08x\n", dma_rd_reg(dma_SG_virtual_address, offset + SG_DA));
   printf("    CONTROL      :0x%08x\n", dma_rd_reg(dma_SG_virtual_address, offset + SG_CONTROL));
   printf("    STATUS       :0x%08x\n", dma_rd_reg(dma_SG_virtual_address, offset + SG_STATUS));
   return (0);
}

void dma_initiate()
{
   if ((ddrOffset + eventLength + ppsLength) > (DDR_MAP_SIZE))
      ddrOffset = 0;
   //ddrOffset = 0;
   /* ========================================================================================== Write SG descriptors */

   if (evtReady == 1)
   { //-- CASE 1: event only DMA transfer
      dma_wr_reg(vadd_sg, SG_DA, addr1 + ddrOffset); //-- RAM header
      dma_wr_reg(vadd_sg, SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0x40 + SG_DA, addr2 + ddrOffset); //-- RAM pre  ch. 1
      dma_wr_reg(vadd_sg, 0x40 + SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0x80 + SG_DA, addr3 + ddrOffset); //-- RAM post ch. 1
      dma_wr_reg(vadd_sg, 0x80 + SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0xC0 + SG_DA, addr4 + ddrOffset); //-- RAM pre  ch. 2
      dma_wr_reg(vadd_sg, 0xC0 + SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0x100 + SG_DA, addr5 + ddrOffset); //-- RAM post ch. 2
      dma_wr_reg(vadd_sg, 0x100 + SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0x140 + SG_DA, addr6 + ddrOffset); //-- RAM pre  ch. 3
      dma_wr_reg(vadd_sg, 0x140 + SG_STATUS, 0x0);

      dma_wr_reg(vadd_sg, 0x180 + SG_DA, addr7 + ddrOffset); //-- RAM post ch. 3
      dma_wr_reg(vadd_sg, 0x180 + SG_STATUS, 0x0);

      if (ppsReady == 1)
      { //-- CASE 2: event + PPS DMA transfer
         dma_wr_reg(vadd_sg, 0x1C0 + SG_DA, addr8 + ddrOffset); //-- RAM pps
         //dma_wr_reg(vadd_sg, 0x1C0 + SG_SA_MSB, 0x0);
         dma_wr_reg(vadd_sg, 0x1C0 + SG_STATUS, 0x0);
         //printf("Transferring event + pps (evtReady,ppsReady)=(%d,%d) ", evtReady,ppsReady);
         ddrOffset = ddrOffset + eventLength + ppsLength;
      }
      else
         ddrOffset = ddrOffset + eventLength;
   }
   else if (ppsReady == 1)
   { //-- CASE 3: PPS only DMA transfer
      dma_wr_reg(vadd_sg, 0x1C0 + SG_DA, addr1 + ddrOffset); //-- RAM pps
      dma_wr_reg(vadd_sg, 0x1C0 + SG_STATUS, 0x0);
      //printf("Transferring pps only (evtReady,ppsReady)=(%d,%d) ", evtReady,ppsReady);
      ddrOffset = ddrOffset + ppsLength;
   }
}

int dma_completion()
{
   // -----------------------------------
   // 1. Verify CDMASR.IDLE = 1
   // -----------------------------------
   do
   {
      idleBit = dma_rd_bit(vadd_cdma, CDMA_SR, 1);
      short_wait();
   }
   while (idleBit == 0);
   //printf("DMA transfer: 1 ");
   // -----------------------------------
   // 2. Clear CDMACR.SGMode = 0
   // -----------------------------------
   dma_wr_bit(vadd_cdma, CDMA_CR, 3, 0);
   short_wait();
   // -----------------------------------
   // 3. Set CDMACR.SGMode = 1
   // -----------------------------------
   dma_wr_bit(vadd_cdma, CDMA_CR, 3, 1);
   short_wait();
   // -----------------------------------
   // 4. Write CURDESC_PNTR register with first BD pointer
   // 5. Enable interrupts in CDMACR
   // 6. Write tail descriptor pointer (start DMA transfer)
   // -----------------------------------
   if (evtReady == 1)
   {
      dma_wr_reg(vadd_cdma, CDMA_CURDESC_PNTR, SG_BASE_ADDR);
      short_wait();
      if (ppsReady == 1)
      { //-- CASE 2: event + PPS DMA transfer
         dma_wr_reg(vadd_cdma, CDMA_CR, IRQ_MASK_AND_SG_evtpps);
         short_wait();
         dma_wr_reg(vadd_cdma, CDMA_TAILDESC_PNTR, SG_BASE_ADDR + 0x1C0);
         short_wait();
      }
      else
      { //-- CASE 1: event only DMA transfer
         dma_wr_reg(vadd_cdma, CDMA_CR, IRQ_MASK_AND_SG_evt);
         short_wait();
         dma_wr_reg(vadd_cdma, CDMA_TAILDESC_PNTR, SG_BASE_ADDR + 0x180);
         short_wait();
      }
   }
   else
   { //-- CASE 3: PPS only DMA transfer
      dma_wr_reg(vadd_cdma, CDMA_CURDESC_PNTR, SG_BASE_ADDR + 0x1C0);
      short_wait();
      dma_wr_reg(vadd_cdma, CDMA_CR, IRQ_MASK_AND_SG_pps);
      short_wait();
      dma_wr_reg(vadd_cdma, CDMA_TAILDESC_PNTR, SG_BASE_ADDR + 0x1C0);
      short_wait();

   }
   // -----------------------------------
   // 7. Wait for the transfer to finish
   // -----------------------------------
   do
   {
      IOC_Irq = dma_rd_bit(vadd_cdma, CDMA_SR, 12);
      short_wait();
      errorIrq = dma_rd_bit(vadd_cdma, CDMA_SR, 14);
      short_wait();
   }
   while ((IOC_Irq == 0) && (errorIrq == 0));
   short_wait();
   if (errorIrq)
      return (-1);
   return (0);
}

void dma_reset()
{
   SGDecErr = dma_rd_bit(vadd_cdma, CDMA_SR, 10); //-- SG engine issues address request to invalid location
   short_wait();
   SGSlvErr = dma_rd_bit(vadd_cdma, CDMA_SR, 9); //-- AXI slave error response received during transfer descriptor read/write
   short_wait();
   SGIntErr = dma_rd_bit(vadd_cdma, CDMA_SR, 8); //-- fetched descriptor has already the complete bit in the status register set (stale)
   short_wait();

   DMADecErr = dma_rd_bit(vadd_cdma, CDMA_SR, 6); //-- AXI Data Mover issues address request to invalid location
   short_wait();
   DMASlvErr = dma_rd_bit(vadd_cdma, CDMA_SR, 5); //-- AXI slave error response received by AXI Data Mover during read/write
   short_wait();
   DMAIntErr = dma_rd_bit(vadd_cdma, CDMA_SR, 4); //-- bytes to transfer inside SG control register is 0, or AXI Data Mover internal error
   short_wait();
   printf("DMA ERROR: SGDecErr %d, SGSlvErr %d, SGIntErr %d; DMADecErr %d, DMASlvErr %d, DMAIntErr %d\n",
          SGDecErr, SGSlvErr, SGIntErr, DMADecErr, DMASlvErr, DMAIntErr);

   //-- DMA halts gracefully, so wait for CMDA shut down
   do
   {
      idleBit = dma_rd_bit(vadd_cdma, CDMA_SR, 1);
      short_wait();
   }
   while (idleBit == 0);

   //-- reset DMA engine to clear error condition
   do
   {
      resetBit = dma_rd_bit(vadd_cdma, CDMA_CR, 2);
      short_wait();
   }
   while (resetBit == 1);
}

int hardware_id()
{
   int fd;
   ssize_t size;
   u_int32_t read_data[50] = {0};
   int32_t index;

   fd = open("/sys/bus/nvmem/devices/zynqmp-nvmem0/nvmem", O_RDWR);
   if (fd <= 0)
      return errno;

   size = pread(fd, (void*) &read_data, 12, 0xC);
   if (size == 12)
   {
      printf("FPGA ID (96 bits): ");
      for (index = (size / 4) - 1; index >= 0; index--)
      {
         printf("%x ", read_data[index]);
      }
      printf("\n\r");
   }
   else
   {
      printf("size != length\n\r");
      return 0;
   }

   close(fd);

   printf("Hardware ID: %x \n", read_data[0]);

   return read_data[0];
}

int scope_open()
{
   uint8_t ch1En = 0, ch2En = 0, ch3En = 0; //-- channel is enabled for readout
   uint32_t ram1Size, ram2Size, ram3Size;
   uint32_t ram4Size, ram5Size, ram6Size;
   uint32_t ram7Size, ram8Size;
   uint32_t ramPreSize, ramPostSize;
   unsigned int addr, page_addr;
   unsigned int page_size = sysconf(_SC_PAGESIZE);

   off_t PSDDR_pbase = DDR_BASE_ADDR;
   off_t RAM1_pbase = RAM1_BASE_ADDR; //-- 1KB   (RAM header)
   off_t RAM2_pbase = RAM2_BASE_ADDR; //-- 16KB  (RAMpre CH1)
   off_t RAM3_pbase = RAM3_BASE_ADDR; //-- 16KB  (RAMpre CH2)
   off_t RAM4_pbase = RAM4_BASE_ADDR; //-- 16KB  (RAMpre CH3)
   off_t RAM5_pbase = RAM5_BASE_ADDR; //-- 16KB  (RAMpost CH1)
   off_t RAM6_pbase = RAM6_BASE_ADDR; //-- 16KB  (RAMpost CH2)
   off_t RAM7_pbase = RAM7_BASE_ADDR; //-- 16KB  (RAMpost CH3)
   off_t RAM8_pbase = RAM8_BASE_ADDR; //-- 1KB   (RAM pps)

   off_t CDMA_pbase = CDMA_BASE_ADDR;
   off_t SG_pbase = SG_BASE_ADDR; //-- 32KB

   if (dev != 0)
      scope_close();

   dev = open("/dev/mem", O_RDWR | O_SYNC);
   if (dev == -1)
   {
      printf("Can't open /dev/mem.\n");
      exit(0);
   }
   printf("/dev/mem opened.\n");

   page_size = sysconf(_SC_PAGESIZE);
   /* ========================================================================================== Map TDAQ DUregs */
   addr = (unsigned int) TDAQDUregs_BASE_ADDR;
   page_addr = (addr & ~(page_size - 1));
   page_offset = addr - page_addr;

   axi_ptr = NULL;
   axi_ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, dev, page_addr);
   if ((long) axi_ptr == -1)
   {
      perror("opening scope\n");
      exit(-1);
   }
   /* ========================================================================================== Map FPGA RAMs */
   // ------------------------------------------------------------- RAM header
   vadd_ram1 = (uint32_t*) mmap(0, RAM_HEADER_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM1_pbase);
   if (vadd_ram1 == (uint32_t*) -1)
   {
      printf("Can't map the RAM1 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM pre CH1
   vadd_ram2 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM2_pbase);
   if (vadd_ram2 == (uint32_t*) -1)
   {
      printf("Can't map the RAM2 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM pre CH2
   vadd_ram3 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM3_pbase);
   if (vadd_ram3 == (uint32_t*) -1)
   {
      printf("Can't map the RAM3 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM pre CH3
   vadd_ram4 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM4_pbase);
   if (vadd_ram4 == (uint32_t*) -1)
   {
      printf("Can't map the RAM4 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM post CH1
   vadd_ram5 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM5_pbase);
   if (vadd_ram5 == (uint32_t*) -1)
   {
      printf("Can't map the RAM5 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM post CH2
   vadd_ram6 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM6_pbase);
   if (vadd_ram6 == (uint32_t*) -1)
   {
      printf("Can't map the RAM6 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM post CH3
   vadd_ram7 = (uint32_t*) mmap(0, RAM_DATA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM7_pbase);
   if (vadd_ram7 == (uint32_t*) -1)
   {
      printf("Can't map the RAM7 memory to user space.\n");
      exit(0);
   }
   // ------------------------------------------------------------- RAM pps
   vadd_ram8 = (uint32_t*) mmap(0, RAM_HEADER_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                RAM8_pbase);
   if (vadd_ram8 == (uint32_t*) -1)
   {
      printf("Can't map the RAM8 memory to user space.\n");
      exit(0);
   }
   /* ========================================================================================== Map PSDDR */
   vadd_psddr = (uint32_t*) mmap(0, DDR_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                 PSDDR_pbase);
   if (vadd_psddr == (uint32_t*) -1)
   {
      printf("Can't map the PSDDR memory to user space.\n");
      exit(0);
   }
   /* ========================================================================================== Map CDMA regs */
   vadd_cdma = (uint32_t*) mmap(0, CDMA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev,
                                CDMA_pbase);
   if (vadd_cdma == (uint32_t*) -1)
   {
      printf("Can't map the CDMA memory to user space.\n");
      exit(0);
   }
   /* ========================================================================================== Map SG regs */
   vadd_sg = (uint32_t*) mmap(0, SG_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev, SG_pbase);
   if (vadd_sg == (uint32_t*) -1)
   {
      printf("Can't map the SG memory to user space.\n");
      exit(0);
   }

   /* ========================================================================================== Write SG descriptors */
   printf("%x %llx\n", (int) vadd_sg, SG_pbase);
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR, SG_BASE_ADDR);
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_SA, RAM1_pbase);
   dma_wr_reg(vadd_sg, SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_CONTROL, 4);
   dma_wr_reg(vadd_sg, SG_STATUS, 0x0);
   // -----------------------------------
   // Reset CMDA
   // -----------------------------------
   dma_wr_bit(vadd_cdma, CDMA_CR, 2, 1);
   do
   {
      resetBit = dma_rd_bit(vadd_cdma, CDMA_CR, 2);
      short_wait();
   }
   while (resetBit == 1);
   printf("DMA initialization: ");
   // -----------------------------------
   // 1. Verify CDMASR.IDLE = 1
   // -----------------------------------
   do
   {
      idleBit = dma_rd_bit(vadd_cdma, CDMA_SR, 1);
      short_wait();
   }
   while (idleBit == 0);
   printf("1 ");
   // -----------------------------------
   // 2. Clear CDMACR.SGMode = 0
   // -----------------------------------
   dma_wr_bit(vadd_cdma, CDMA_CR, 3, 0);
   printf("2 ");
   // -----------------------------------
   // 3. Set CDMACR.SGMode = 1
   // -----------------------------------
   dma_wr_bit(vadd_cdma, CDMA_CR, 3, 1);
   printf("3 ");
   // -----------------------------------
   // 4. Write CURDESC_PNTR register with first BD pointer
   // -----------------------------------
   dma_wr_reg(vadd_cdma, CDMA_CURDESC_PNTR, SG_BASE_ADDR);
   printf("4 ");
   // -----------------------------------
   // 5. Enable interrupts in CDMACR
   // -----------------------------------
   dma_wr_reg(vadd_cdma, CDMA_CR, IRQ_MASK_AND_SG_pps);
   printf("5 ");
   // -----------------------------------
   // 6. Write tail descriptor pointer (start DMA transfer)
   // -----------------------------------
   dma_wr_reg(vadd_cdma, CDMA_TAILDESC_PNTR, SG_BASE_ADDR);
   printf("6 ");
   // -----------------------------------
   // 7. Wait for the transfer to finish
   // -----------------------------------
   do
   {
      IOC_Irq = dma_rd_bit(vadd_cdma, CDMA_SR, 12);
      short_wait();
      errorIrq = dma_rd_bit(vadd_cdma, CDMA_SR, 14);
      short_wait();
   }
   while ((IOC_Irq == 0) && (errorIrq == 0));
   printf("7 ");

   if (errorIrq)
   {
      dma_reset();
   }
   else
   {
      // -----------------------------------
      // 8. Wait for evtReady or ppsReady
      // -----------------------------------
      do
      {
         evtReady = DUreg_rd_bit(0x0050, 0);
         short_wait();
         ppsReady = DUreg_rd_bit(0x0050, 1);
         short_wait();
      }
      while ((evtReady == 0) && (ppsReady == 0));
      printf("8 ");
      // -----------------------------------
      // 9. Clear the IOC interrupt bit
      // -----------------------------------
      dma_wr_bit(vadd_cdma, CDMA_SR, 12, 1);
      printf("9 ... done\n");
   }
   /* ========================================================================================== Prepare SG descriptors */
   ram1Size = 584; //-- header transfer size in Bytes
   postOverlap = 4 * DUreg_rd_excerpt(0x0010, 17, 28); //-- postOverlap transfer size per channel in Bytes
   preOverlap = 4 * DUreg_rd_excerpt(0x0010, 5, 16); //-- preOverlap transfer size per channel in Bytes
   Overlap = 4 * DUreg_rd_excerpt(0x0010, 0, 4); //-- Overlap transfer size per channel in Bytes
   printf("Windows (Bytes) %d %d %d\n", preOverlap, Overlap, postOverlap);
   ram8Size = 88; //-- pps transfer size in Bytes
   if (DUreg_rd_excerpt(0x0014, 1, 4) != 0)
      ch1En = 1; //-- channel 1 is enabled for readout
   if (DUreg_rd_excerpt(0x0014, 6, 9) != 0)
      ch2En = 1; //-- channel 2 is enabled for readout
   if (DUreg_rd_excerpt(0x0014, 11, 14) != 0)
      ch3En = 1; //-- channel 3 is enabled for readout
   ramPreSize = preOverlap + Overlap;
   ramPostSize = postOverlap;
   //ramPostSize = ramPreSize + postOverlap;
   ram2Size = 4;
   ram3Size = 4;
   ram4Size = 4;
   ram5Size = 4;
   ram6Size = 4;
   ram7Size = 4;
   if (ch1En == 1)
   {
      ram2Size = ramPreSize;
      ram5Size = ramPostSize;
   }
   if (ch2En == 1)
   {
      ram3Size = ramPreSize;
      ram6Size = ramPostSize;
   }
   if (ch3En == 1)
   {
      ram4Size = ramPreSize;
      ram7Size = ramPostSize;
   }
   addr1 = PSDDR_pbase; //-- header
   addr2 = addr1 + ram1Size; //-- RAMpre  ch1
   addr3 = addr2 + ch1En * ram2Size; //-- RAMpost ch1
   addr4 = addr3 + ch1En * ram5Size; //-- RAMpre  ch2
   addr5 = addr4 + ch2En * ram3Size; //-- RAMpost ch2
   addr6 = addr5 + ch2En * ram6Size; //-- RAMpre  ch3
   addr7 = addr6 + ch3En * ram4Size; //-- RAMpost ch3
   addr8 = addr7 + ch3En * ram7Size; //-- RAM pps

   eventLength = ram1Size + ch1En * (ram2Size + ram3Size) + ch2En * (ram4Size + ram5Size)
                                 + ch3En * (ram6Size + ram7Size);
   printf("Scope_open: Event Length =  %d\n", eventLength / 4);
   ppsLength = ram8Size;

   //-- 0x00; 0x40; 0x80; 0xC0; 0x100; 0x140; 0x180; 0x1C0; 0x200;
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x40); //-- RAM header
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_SA, RAM1_pbase);
   dma_wr_reg(vadd_sg, SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_CONTROL, ram1Size);
   dma_wr_reg(vadd_sg, SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x40 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x80); //-- RAM pre ch. 1
   dma_wr_reg(vadd_sg, 0x40 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_SA, RAM2_pbase);
   dma_wr_reg(vadd_sg, 0x40 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x40 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_CONTROL, ram2Size);
   dma_wr_reg(vadd_sg, 0x40 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0xC0 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x100); //-- RAM pre ch. 2
   dma_wr_reg(vadd_sg, 0xC0 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_SA, RAM3_pbase);
   dma_wr_reg(vadd_sg, 0xC0 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0xC0 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_CONTROL, ram3Size);
   dma_wr_reg(vadd_sg, 0xC0 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x140 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x180); //-- RAM pre ch. 3
   dma_wr_reg(vadd_sg, 0x140 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_SA, RAM4_pbase);
   dma_wr_reg(vadd_sg, 0x140 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x140 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_CONTROL, ram4Size);
   dma_wr_reg(vadd_sg, 0x140 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x80 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0xC0); //-- RAM post ch. 1
   dma_wr_reg(vadd_sg, 0x80 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_SA, RAM5_pbase);
   dma_wr_reg(vadd_sg, 0x80 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x80 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_CONTROL, ram5Size);
   dma_wr_reg(vadd_sg, 0x80 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x100 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x140); //-- RAM post ch. 2
   dma_wr_reg(vadd_sg, 0x100 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_SA, RAM6_pbase);
   dma_wr_reg(vadd_sg, 0x100 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x100 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_CONTROL, ram6Size);
   dma_wr_reg(vadd_sg, 0x100 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x180 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x1C0); //-- RAM post ch. 3
   dma_wr_reg(vadd_sg, 0x180 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_SA, RAM7_pbase);
   dma_wr_reg(vadd_sg, 0x180 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x180 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_CONTROL, ram7Size);
   dma_wr_reg(vadd_sg, 0x180 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x1C0 + SG_NXTDESC_PNTR, SG_BASE_ADDR); //-- RAM pps
   dma_wr_reg(vadd_sg, 0x1C0 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_SA, RAM8_pbase);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_CONTROL, ram8Size);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_STATUS, 0x0);

   ddrOffset = 0;
   *shm_gps.next_read = 0;
   *shm_gps.next_write = 0;

   return (1);
}

void scope_close()
{
   printf("scope close\n");
   if (munmap(vadd_psddr, DDR_MAP_SIZE) == -1)
   {
      printf("Can't unmap DDR  memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_sg, SG_MAP_SIZE) == -1)
   {
      printf("Can't unmap SG   memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_cdma, CDMA_MAP_SIZE) == -1)
   {
      printf("Can't unmap CDMA memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram1, RAM_HEADER_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM1 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram2, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM2 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram3, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM3 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram4, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM4 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram5, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM5 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram6, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM6 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram7, RAM_DATA_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM7 memory from user space.\n");
      exit(0);
   }
   if (munmap(vadd_ram8, RAM_HEADER_MAP_SIZE) == -1)
   {
      printf("Can't unmap RAM8 memory from user space.\n");
      exit(0);
   }
   close(dev);
   dev = 0;

   TFLT_delete(&G_ptflt1);
}

void scope_start_run()
{
   if (axi_ptr == NULL)
      return;
   //scope_flush();
}

void scope_stop_run()
{
   printf("Scope stop run\n");
   if (axi_ptr == NULL)
      return;
   //scope_flush();
}

void scope_initialize()
{
   uint8_t ch1En = 0, ch2En = 0, ch3En = 0; //-- channel is enabled for readout
   uint32_t ram1Size, ram2Size, ram3Size;
   uint32_t ram4Size, ram5Size, ram6Size;
   uint32_t ram7Size, ram8Size;
   uint32_t ramPreSize, ramPostSize;

   off_t PSDDR_pbase = DDR_BASE_ADDR;
   off_t RAM1_pbase = RAM1_BASE_ADDR; //-- 1KB   (RAM header)
   off_t RAM2_pbase = RAM2_BASE_ADDR; //-- 16KB  (RAMpre CH1)
   off_t RAM3_pbase = RAM3_BASE_ADDR; //-- 16KB  (RAMpre CH2)
   off_t RAM4_pbase = RAM4_BASE_ADDR; //-- 16KB  (RAMpre CH3)
   off_t RAM5_pbase = RAM5_BASE_ADDR; //-- 16KB  (RAMpost CH1)
   off_t RAM6_pbase = RAM6_BASE_ADDR; //-- 16KB  (RAMpost CH2)
   off_t RAM7_pbase = RAM7_BASE_ADDR; //-- 16KB  (RAMpost CH3)
   off_t RAM8_pbase = RAM8_BASE_ADDR; //-- 1KB   (RAM pps)

   //off_t CDMA_pbase  = CDMA_BASE_ADDR;
   //off_t SG_pbase    = SG_BASE_ADDR;       //-- 32KB
   /* ========================================================================================== Prepare SG descriptors */
   ram1Size = 584; //-- header transfer size in Bytes
   postOverlap = 4 * DUreg_rd_excerpt(0x0010, 17, 28); //-- postOverlap transfer size per channel in Bytes
   preOverlap = 4 * DUreg_rd_excerpt(0x0010, 5, 16); //-- preOverlap transfer size per channel in Bytes
   Overlap = 4 * DUreg_rd_excerpt(0x0010, 0, 4); //-- Overlap transfer size per channel in Bytes
   //printf("Windows (Bytes) %d %d %d\n",preOverlap,Overlap,postOverlap);
   ram8Size = 88; //-- pps transfer size in Bytes
   if (DUreg_rd_excerpt(0x0014, 1, 4) != 0)
      ch1En = 1; //-- channel 1 is enabled for readout
   if (DUreg_rd_excerpt(0x0014, 6, 9) != 0)
      ch2En = 1; //-- channel 2 is enabled for readout
   if (DUreg_rd_excerpt(0x0014, 11, 14) != 0)
      ch3En = 1; //-- channel 3 is enabled for readout
   ramPreSize = preOverlap + Overlap;
   ramPostSize = postOverlap;
   //ramPostSize = ramPreSize + postOverlap;
   ram2Size = 4;
   ram3Size = 4;
   ram4Size = 4;
   ram5Size = 4;
   ram6Size = 4;
   ram7Size = 4;
   if (ch1En == 1)
   {
      ram2Size = ramPreSize;
      ram5Size = ramPostSize;
   }
   if (ch2En == 1)
   {
      ram3Size = ramPreSize;
      ram6Size = ramPostSize;
   }
   if (ch3En == 1)
   {
      ram4Size = ramPreSize;
      ram7Size = ramPostSize;
   }
   addr1 = PSDDR_pbase; //-- header
   addr2 = addr1 + ram1Size; //-- RAMpre  ch1
   addr3 = addr2 + ch1En * ram2Size; //-- RAMpost ch1
   addr4 = addr3 + ch1En * ram5Size; //-- RAMpre  ch2
   addr5 = addr4 + ch2En * ram3Size; //-- RAMpost ch2
   addr6 = addr5 + ch2En * ram6Size; //-- RAMpre  ch3
   addr7 = addr6 + ch3En * ram4Size; //-- RAMpost ch3
   addr8 = addr7 + ch3En * ram7Size; //-- RAM pps

   eventLength = ram1Size + ch1En * (ram2Size + ram3Size) + ch2En * (ram4Size + ram5Size)
                                 + ch3En * (ram6Size + ram7Size);
   //printf("Scope_open: Event Length =  %d\n",eventLength/4);
   ppsLength = ram8Size;

   //-- 0x00; 0x40; 0x80; 0xC0; 0x100; 0x140; 0x180; 0x1C0; 0x200;
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x40); //-- RAM header
   dma_wr_reg(vadd_sg, SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_SA, RAM1_pbase);
   dma_wr_reg(vadd_sg, SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, SG_CONTROL, ram1Size);
   dma_wr_reg(vadd_sg, SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x40 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x80); //-- RAM pre ch. 1
   dma_wr_reg(vadd_sg, 0x40 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_SA, RAM2_pbase);
   dma_wr_reg(vadd_sg, 0x40 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x40 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x40 + SG_CONTROL, ram2Size);
   dma_wr_reg(vadd_sg, 0x40 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0xC0 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x100); //-- RAM pre ch. 2
   dma_wr_reg(vadd_sg, 0xC0 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_SA, RAM3_pbase);
   dma_wr_reg(vadd_sg, 0xC0 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0xC0 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0xC0 + SG_CONTROL, ram3Size);
   dma_wr_reg(vadd_sg, 0xC0 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x140 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x180); //-- RAM pre ch. 3
   dma_wr_reg(vadd_sg, 0x140 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_SA, RAM4_pbase);
   dma_wr_reg(vadd_sg, 0x140 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x140 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x140 + SG_CONTROL, ram4Size);
   dma_wr_reg(vadd_sg, 0x140 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x80 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0xC0); //-- RAM post ch. 1
   dma_wr_reg(vadd_sg, 0x80 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_SA, RAM5_pbase);
   dma_wr_reg(vadd_sg, 0x80 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x80 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x80 + SG_CONTROL, ram5Size);
   dma_wr_reg(vadd_sg, 0x80 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x100 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x140); //-- RAM post ch. 2
   dma_wr_reg(vadd_sg, 0x100 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_SA, RAM6_pbase);
   dma_wr_reg(vadd_sg, 0x100 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x100 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x100 + SG_CONTROL, ram6Size);
   dma_wr_reg(vadd_sg, 0x100 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x180 + SG_NXTDESC_PNTR, SG_BASE_ADDR + 0x1C0); //-- RAM post ch. 3
   dma_wr_reg(vadd_sg, 0x180 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_SA, RAM7_pbase);
   dma_wr_reg(vadd_sg, 0x180 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x180 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x180 + SG_CONTROL, ram7Size);
   dma_wr_reg(vadd_sg, 0x180 + SG_STATUS, 0x0);

   dma_wr_reg(vadd_sg, 0x1C0 + SG_NXTDESC_PNTR, SG_BASE_ADDR); //-- RAM pps
   dma_wr_reg(vadd_sg, 0x1C0 + SG_NXTDESC_PNTR_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_SA, RAM8_pbase);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_SA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_DA, PSDDR_pbase);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_DA_MSB, 0x0);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_CONTROL, ram8Size);
   dma_wr_reg(vadd_sg, 0x1C0 + SG_STATUS, 0x0);

   /* Init TRIGGER 2*/
   scope_create_thread_T2();
}

/**
 *  1) Add GPS datation in event message
 *  2) Add event in ring buffer for evaluation
 *  3) Inspect event evaluated
 */
int scope_read_event(uint32_t index)
{
   static int S_idx_write = 0;

   /** temp fix for ARG */
//struct timeval tv;
   struct tm tt;
   int length;
   double fracsec;
   uint32_t CTP, CTD;
   uint32_t* sec;
   uint32_t* evt = &vadd_psddr[index];

   /* Add GPS datation in event message */
   length = evt[0] >> 16;
   if (length <= 0)
      return (-2);
   evt[EVT_VERSION] = (evt[EVT_VERSION] & 0xffff0000) + (DUDAQ_VERSION & 0xff);
   if (hw_id == 0)
      hw_id = hardware_id();
   evt[EVT_HARDWARE_ID] = hw_id;
   //printf("Read Event; PPS = %d sec = %d CTP = %d CTD = %d Offset = %g Trigger %04x\n",
   //       evt[EVT_PPS_ID],evt[EVT_WEEKTIME],evt[EVT_CTP],evt[EVT_CTD],*(float *)&evt[EVT_OFFSET],
   //       evt[EVT_TRIGGER_STAT]&0xffff);
   sec = (uint32_t*) &evt[EVT_SECOND];
   tt.tm_sec = ((evt[EVT_SECMINHOUR] >> 16) & 0xff) - (evt[EVT_WEEKOFFSET] & 0xffff); // Convert GPS in a number of seconds
   tt.tm_min = (evt[EVT_SECMINHOUR] >> 8) & 0xff;
   tt.tm_hour = (evt[EVT_SECMINHOUR]) & 0xff;
   tt.tm_mday = (evt[EVT_DAYMONTH] >> 24) & 0xff;
   tt.tm_mon = ((evt[EVT_DAYMONTH] >> 16) & 0xff) - 1;
   tt.tm_year = evt[EVT_YEAR] - 1900;
   CTP = evt[EVT_CTP];
   CTD = evt[EVT_CTD];
   if ((CTP & 0x80000000) || (CTD & 0x80000000))
   {
      CTP &= 0x7fffffff;
      *(uint32_t*) &evt[EVT_CTP] = CTP;
      CTD &= 0x7fffffff;
      *(uint32_t*) &evt[EVT_CTD] = CTD;
   }
   *sec = (unsigned int) timegm(&tt); //previous PPS was used
   if (evt[EVT_PPS_ID] > prev_ppsid)
      *sec = *sec + 1;
   //printf("scope_read_evt: Second = %d --> %u\n",tt.tm_sec,*sec);
   //if(*sec > 1690000000 && *sec < 2000000000)
   last_sec = *sec;
   fracsec = (double) (CTD) / (double) (CTP);
   evt[EVT_NANOSEC] = 1.E9 * fracsec;
   evt[EVT_STATION_ID] = station_id;

   /* Add event in ring buffer for evaluation */
   if (Gp_rbuftrig[S_idx_write]->nb_write > 0)
   {
      RBE_write(Gp_rbuftrig[S_idx_write], evt);
      /*prendre le mutex des le debut ?*/
      RBE_after_write(Gp_rbuftrig[S_idx_write]);
   }
   else
   {
      printf("\nT2 ring buffer saturated ! ");
   }

   /* switch thread write */
   if (S_idx_write == 0)
   {
      S_idx_write = 1;
   }
   else
   {
      S_idx_write = 0;
   }

   return (SCOPE_EVENT); // success!
}

int32_t scope_read_pps(uint32_t index)
{
   uint32_t* pps = &vadd_psddr[index];
   //printf("PPS reading: %d sec = %d CTP = %d Offset = %g\n",
   //       pps[PPS_ID],pps[PPS_WEEKTIME],pps[PPS_CTP],*(float *)&pps[PPS_OFFSET]);
   prev_ppsid = pps[PPS_ID];
   shm_gps.Ubuf[*shm_gps.next_write] = index;
   *shm_gps.next_write = *(shm_gps.next_write) + 1;
   if (*shm_gps.next_write >= *shm_gps.nbuf)
      *shm_gps.next_write = 0;
   // scope_calc_t3nsec();
   return (SCOPE_GPS);
}

int scope_read()
{

   dma_initiate();
   if (dma_completion() == -1)
   {
      dma_reset();
   }
   else
   {
      if (evtReady == 1)
         scope_read_event(ddrPrevOffset / 4);
      if (ppsReady == 1)
      {
         if (evtReady == 1)
            scope_read_pps((ddrPrevOffset + eventLength) / 4);
         else
            scope_read_pps(ddrPrevOffset / 4);
      }
      ddrPrevOffset = ddrOffset;
      // -----------------------------------
      // 8. Wait for evtReady or ppsReady
      // -----------------------------------
      do
      {
         evtReady = DUreg_rd_bit(0x0050, 0);
         short_wait();
         ppsReady = DUreg_rd_bit(0x0050, 1);
         short_wait();
      }
      while ((evtReady == 0) && (ppsReady == 0));
      short_wait();
      // -----------------------------------
      // 9. Clear the IOC interrupt bit
      // -----------------------------------
      dma_wr_bit(vadd_cdma, CDMA_SR, 12, 1);
      short_wait();
   }
   return (0);
}

int scope_calc_t3nsec()
{
   return (1);
}

