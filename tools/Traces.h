/* First define the header words */
#define FILE_HDR_LENGTH          0
#define FILE_HDR_RUNNR           1
#define FILE_HDR_RUN_MODE        2
#define FILE_HDR_SERIAL          3
#define FILE_HDR_FIRST_EVENT     4
#define FILE_HDR_FIRST_EVENT_SEC 5
#define FILE_HDR_LAST_EVENT      6
#define FILE_HDR_LAST_EVENT_SEC  7
#define FILE_HDR_ADDITIONAL      8 //start of additional info to be defined

#define EVENT_HDR_LENGTH          0
#define EVENT_HDR_RUNNR           1
#define EVENT_HDR_EVENTNR         2
#define EVENT_HDR_T3EVENTNR       3
#define EVENT_HDR_FIRST_DU        4
#define EVENT_HDR_EVENT_SEC       5
#define EVENT_HDR_EVENT_NSEC      6
#define EVENT_HDR_EVENT_TYPE      7
#define EVENT_HDR_EVENT_VERS      8
#define EVENT_HDR_NDU             9 //start of additional info to be defined
#define EVENT_HDR_AD1            10 //                    info to be defined
#define EVENT_DU                  11

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
#define EVT_FPGA_TEMP     14
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
#define EVT_LATITUDE      31
#define EVT_LONGITUDE     33
#define EVT_ALTITUDE      35
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
