/// @file DU.h
/// @brief few definitions to be used for the main program
/// @author C. Timmermans, Nikhef/RU

#include<sys/time.h>

#define DUDAQ_VERSION 2
#define MAX_INP_MSG 20000 //!< maximum size (in shorts) of the input data (should be big enough for the configuation data)
#define MAX_OUT_MSG 20000 //!< maximum size (in shorts) of the output data (should be big enough for the T2 data)
#define MAX_T2 3000 //! max size for T2's, corresponds to the Adaq

#define MSGSTOR 100 //!< number of messages in shared memory

#define DU_PORT 5001 //!< socket port number of the detector unit software


