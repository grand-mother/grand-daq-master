# read "AXI memory"

"share memory" between CPU and FPGA defined in scope_open() in scope.c

```      
  #define DEVFILE "/dev/mem" //!< Device for talking to the FPGA
  
  if ((dev = open (DEVFILE, O_RDWR)) == -1)
  {
    fprintf (stderr, "Error opening scope device file %s for read/write\n",
    DEVFILE);
    return (-1);
  }
  addr = (unsigned int) TDAQ_BASE;
  page_addr = (addr & ~(page_size - 1));
  page_offset = addr - page_addr;

  axi_ptr = mmap (NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, dev, page_addr);
```

# Main

In dudaq.c, creating of 5 share memory
* shm_struct shm_ev;     //!< shared memory containing all event info, including read/write pointers
* shm_struct shm_gps;     //!< shared memory containing all GPS info, including read/write pointers
* shm_struct shm_ts;     //!< shared memory containing all timestamp info, including read/write pointers
* shm_struct shm_cmd;    //!< shared memory containing all command info, including read/write pointers
* shm_struct shm_mon;     //!< shared memory containing monitoring info

## share memory events

process du_scope_main()
