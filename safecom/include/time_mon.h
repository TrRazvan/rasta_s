#ifndef TIME_MON_H
#define TIME_MON_H

#include <stdint.h>

typedef struct {
    uint32_t Th;    /* Configuration parameter: Time period for sending heartbeats (heartbeats are only sent if 
                        there is no data waiting to be transmitted) */
    uint32_t Tmax;  /* Configuration parameter: maximum accepted age of a message */
    uint32_t Tseq;  /* Configuration parameter of the redundancy layer: Monitored time interval for how long a 
                        message which was received in the redundancy layer out of sequence is kept in the queue. After that, 
                        this message is passed on to the next higher layer*/
} TimeoutsConfig;

/* Type definition for the get time function pointer */
typedef uint32_t (*GetTimestamp)();

typedef struct {
    TimeoutsConfig timeouts;
    // uint32_t Tlocal;    /* Local time (timestamp at the time of analysis) */
    GetTimestamp Tlocal;
    int32_t Ti;        /* Monitoring time for incoming messages (calculated dynamically) */
    uint32_t Trtd;      /* Round trip delay of a message */
    uint32_t Talive;    /* Tlocal - CTSR : is calculated upon receipt of a message relevant to time monitoring 
                            (provides a statement to what extent the adaptive channel monitoring has exhausted the quota Tmax) */
    uint32_t TA;        /* Transmission time from party A to party B; it includes manufacturer-specific times for generating and 
                            sending the message, the network delay times and the manufacturer-specific time for processing message upon receipt */
    uint32_t TB;        /* Transmission time from party B to party A, in analogy to TA */
} TimeMonitoring;

uint32_t GetCurrentTimestamp(void);

#endif