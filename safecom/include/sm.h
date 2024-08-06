#ifndef SM_H
#define SM_H

#include <stdint.h>
#include <stdbool.h>
#include "pdu.h"
#include "types.h"

#define TMAX    500U /* TODO: RTR - Define TMP_MAX */

/* Define states of the state machine */
typedef enum {
    STATE_CLOSED,
    STATE_DOWN,
    STATE_START,
    STATE_UP,
    STATE_RETR_REQ,
    STATE_RETR_RUN
} State;

/* Define events that trigger state transitions */
typedef enum {
    EVENT_OPEN_CONN,
    EVENT_CLOSE_CONN,
    EVENT_SEND_DATA,
    EVENT_RECV_CONN_REQ,
    EVENT_RECV_CONN_RESP,
    EVENT_RECV_RETR_REQ,
    EVENT_RECV_RETR_RESP,
    EVENT_RECV_DISC_REQ,
    EVENT_RECV_HB,
    EVENT_RECV_DATA,
    EVENT_RECV_RETR_DATA,
    EVENT_TH_ELAPSED,
    EVENT_TI_ELAPSED
} Event;


typedef enum {
    ROLE_CLIENT,
    ROLE_SERVER
} SmRole;

/* State machine context structure */
typedef struct SmType SmType;

/* Type definition for the state handler function pointer */
typedef void (*EventHandler)(SmType *self, Event event, PDU_S *pdu);

/* State machine context definition */
struct SmType {
    MsgId_t channel; /* The channel/connection/msg_id number */
    State state;    /* The state of the state machine */
    SmRole role;    /* The state machine can act like a client or like a server */
    int32_t snr;    /* Receive sequence number (i.e. the expected sequence number of the next received protocol data unit) */
    int32_t snt;    /* Send sequence number (i.e. the sequence number of the protocol data unit to be sent next) */
    int32_t snpdu;  /* Sequence number field of a protocol data unit */
    int32_t cst;    /* Sequence number to be confirmed (which is transmitted at the next protocol data unit to be sent) */
    int32_t csr;    /* Last received confirmed sequence number */
    int32_t tsr;    /* Timestamp of the last formally correct message received */
    int32_t ctsr;   /* Confirmed timestamp of the last received message relevant to time monitoring */
    int32_t cspdu;  /* Confirmed sequence number field of a protocol data unit */
    int32_t ctspdu; /* Confirmed timestamp (field in the protocol data unit) */
    int32_t tspdu;  /* Timestamp for the sender (field in the protocol data unit) */
    EventHandler handle_event;
};

/**
 * @brief Initializes the RastaS module.
 *
 * @param[in]   self Pointer to my RastaS structure handle.
 * 
 * @retval - `OK`      If the initialization was done successfully.
 * @retval - `NOT_OK`  If the initialization was not done successfully.
 */
StdRet_t Sm_Init(SmType *self);

// StdRet_t Rass_OpenConnection(RassId_t rassIdRem);

void Sm_HandleEvent(SmType *self, Event event, PDU_S *pdu);

#endif /* SM_H */
