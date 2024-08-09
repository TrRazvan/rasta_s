#include "sm.h"
#include "assert.h"
#include "log.h"

/* Private function prototypes */
static void set_initial_values(SmType *self);
static void close_connection(SmType *self);
static void process_regular_receipt(SmType *self);
static void handle_closed(SmType *self, const Event event, const PDU_S recv_pdu);
static void handle_down(SmType *self, const Event event, const PDU_S recv_pdu);
static void handle_start(SmType *self, const Event event, const PDU_S recv_pdu);
static void handle_up(SmType *self, const Event event, const PDU_S recv_pdu);
static void handle_retr_req(SmType *self, const Event event, const PDU_S recv_pdu);
static void handle_retr_run(SmType *self, const Event event, const PDU_S recv_pdu);
static bool check_seq_confirmed_timestamp(SmType *self);
static bool check_version(const PDU_S recv_pdu);

static bool check_seq_confirmed_timestamp(SmType *self)
{
    assert(self != NULL);

    bool ret = false;

    /* TODO: RTR - Define TMAX */
    if(((self->ctspdu - self->ctsr) >= 0) && ((self->ctspdu - self->ctsr) < TMAX))
    {
        ret=true;
    }
    else
    {
        ret=false;
    }
    return ret;
}

static bool check_version(const PDU_S pdu)
{
    assert(pdu.payload != NULL);
    
    if ((pdu.payload[0] == ((PROTOCOL_VERSION >> SHIFT_3_BYTES) & 0xFF)) &&
        (pdu.payload[1] == ((PROTOCOL_VERSION >> SHIFT_2_BYTES) & 0xFF)) &&
        (pdu.payload[2] == ((PROTOCOL_VERSION >> SHIFT_1_BYTES) & 0xFF)) &&
        (pdu.payload[3] == (PROTOCOL_VERSION & 0xFF)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* Private functions */
static void set_initial_values(SmType *self)
{
    assert(self != NULL);

    self->snr = 0;
    self->snt = 0;
    self->snpdu = 0;
    self->cst = 0;
    self->csr = 0;
    self->tsr = 0;
    self->ctsr = 0;
    self->cspdu = 0;
    self->ctspdu = 0;
    self->tspdu = 0;
}

static void process_regular_receipt(SmType *self)
{
    assert(self != NULL);
    self->snr = self->snpdu + 1;
    self->cst = self->snpdu;
    self->csr = self->cspdu;
    self->tsr = self->tspdu;
    self->ctsr = self->cspdu;
}

static void handle_closed(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);
    UNUSED(recv_pdu);

    PDU_S pdu_to_send = { 0 };

    switch (event) {
        case EVENT_OPEN_CONN:
            if(self->role == ROLE_SERVER)
            {
                self->snt = rand(); /* Random value for SNT */
                self->state = STATE_DOWN;
            }
            else if(self->role == ROLE_CLIENT)
            {
                self->snt = rand(); /* Random value for SNT */
                self->cst = 0;
                self->ctsr = 0; /* TODO: RTR - Tlocal */

                pdu_to_send = ConnReq(*self);
                /* TODO: RTR - Send ConnReq */

                self->state = STATE_START;
            }
            break;
        default:
            /* No action for other events in STATE_CLOSED */
            break;
    }
}

static void handle_down(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };
    
    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_CLOSE_CONN:
        case EVENT_SEND_DATA:
            close_connection(self);
            break;

        case EVENT_RECV_CONN_REQ:
            if (check_version(recv_pdu)) 
            {
                self->csr = self->snt - 1;
                self->ctsr = 0; /* TODO: RTR - Tlocal */
                self->state = STATE_START;

                pdu_to_send = ConnResp(*self);
                /* TODO: RTR - Send ConnResp */
            }
            else
            {
                close_connection(self);

                pdu_to_send = DiscReq(6, NO_DETAILED_REASON, *self);
                /* TODO: RTR - Send DiscReq(6) */
            }
            break;
        default:
            /* No action for other events in STATE_DOWN */
            break;
    }
}

static void handle_start(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };

    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_SEND_DATA:
            close_connection(self);

            pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(5) */
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_RETR_REQ:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_DATA:
        case EVENT_RECV_RETR_DATA:
            close_connection(self);

            pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(2) */
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(0, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(0) */
            break;


        case EVENT_RECV_CONN_RESP:
            if (self->role == ROLE_SERVER)
            {
                close_connection(self);

                pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
                /* TODO: RTR - Send DiscReq(2) */
            }
            else if (self->role == ROLE_CLIENT)
            {
                if (check_version(recv_pdu)) 
                {
                    self->state = STATE_UP;

                    pdu_to_send = HB(*self);
                    /* TODO: RTR - Send HB */
                }
                else
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(6, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(6) */
                }
            }
            break;

            case EVENT_RECV_DISC_REQ:
                close_connection(self);
            break;

            case EVENT_RECV_HB:
                if (self->role == ROLE_SERVER)
                {
                    /* Checking the sequence number SNinSeq == true */
                    if (self->snr == self->snpdu)
                    {
                        /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                        if (check_seq_confirmed_timestamp(self))
                        {
                            process_regular_receipt(self);
                            self->state = STATE_UP;
                        }
                        else
                        {
                            close_connection(self);

                            pdu_to_send = DiscReq(8, NO_DETAILED_REASON, *self);
                            /* TODO: RTR - Send DiscReq(8) */
                        }
                    }
                    else 
                    {
                        close_connection(self);

                        pdu_to_send = DiscReq(3, NO_DETAILED_REASON, *self);
                        /* TODO: RTR - Send DiscReq(3) */
                    }
                }
                else if (self->role == ROLE_CLIENT)
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(2) */
                }
            break;

        default:
            /* No action for other events in STATE_START */
            break;
    }
}

static void handle_up(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);
    UNUSED(recv_pdu);
    
    PDU_S pdu_to_send = { 0 };

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(5) */
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(0, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(0) */
            break;

        case EVENT_SEND_DATA:
            /* TODO: RTR - Send Data */
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_RETR_DATA:
            close_connection(self);

            pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(2) */
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* TODO: RTR - all unconfirmed payload data available */
                if (1) 
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                } 
                else 
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(7, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(7) */
                }
            }
            else
            {
                /* TODO: RTR - all unconfirmed payload data available */
                if (1) 
                {
                    self->csr = self->cspdu;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                }
                else 
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(7, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(7) */
                }
            }
            break;

        case EVENT_RECV_HB:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self))
                {
                    process_regular_receipt(self);
                }
                else
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(8, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(8) */
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                pdu_to_send = RetrReq(*self);
                /* TODO: RTR - Send RetrReq */
            }
            break;

        case EVENT_RECV_DATA:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self))
                {
                    process_regular_receipt(self);
                }
                else
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(8, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(8) */
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;
                
                pdu_to_send = RetrReq(*self);
                /* TODO: RTR - Send RetrReq */
            }
            break;
        default:
            /* No action for other events in STATE_UP */
            break;
    }
}

static void handle_retr_req(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);
    UNUSED(recv_pdu);
    
    PDU_S pdu_to_send = { 0 };

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(5) */
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(0, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(0) */
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
            close_connection(self);

            pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(2) */
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* TODO: RTR - all unconfirmed payload data available */
                if (1) 
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                } 
                else 
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(7, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(7) */
                }
            }
            else
            {
                /* TODO: RTR - all unconfirmed payload data available */
                if (1) 
                {
                    self->csr = self->cspdu;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                }
                else 
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(7, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(7) */
                }
            }
            break;

        case EVENT_SEND_DATA:
            self->state = STATE_RETR_REQ;
            /* TODO: RTR - Send Data */
            break;

        case EVENT_RECV_RETR_RESP:
            self->state = STATE_RETR_RUN;
            break;

        case EVENT_RECV_HB:
        case EVENT_RECV_DATA:
        case EVENT_RECV_RETR_DATA:
            self->state = STATE_RETR_REQ;
            break;

        default:
            /* No action for other events in STATE_RETR_REQ */
            break;
    }
}

static void handle_retr_run(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);
    UNUSED(recv_pdu);
    
    PDU_S pdu_to_send = { 0 };

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(5) */
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            pdu_to_send = DiscReq(0, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(0) */
            break;

        case EVENT_SEND_DATA:
            self->state = STATE_RETR_RUN;
            /* TODO: RTR - Send Data */
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
            close_connection(self);

            pdu_to_send = DiscReq(5, NO_DETAILED_REASON, *self);
            /* TODO: RTR - Send DiscReq(2) */
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                close_connection(self);

                pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
                /* TODO: RTR - Send DiscReq(2) */
            }
            else
            {
                /* TODO: RTR - all unconfirmed payload data available */
                if (1)
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp RetrData(s) HB or Data RetrReq */
                }
                else
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(7) */
                }
            }
            break;

        case EVENT_RECV_HB:
        case EVENT_RECV_DATA:
        case EVENT_RECV_RETR_DATA:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self))
                {
                    self->state = STATE_UP;
                } 
                else
                {
                    close_connection(self);

                    pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
                    /* TODO: RTR - Send DiscReq(8) */
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                pdu_to_send = RetrReq(*self);
                /* TODO: RTR - Send RetrReq */
            }
            break;

        default:
            /* No action for other events in STATE_RETR_RUN */
            break;
    }
}

static void close_connection(SmType *self)
{
    assert(self != NULL);
    self->cst = self->snpdu;
    self->state = STATE_CLOSED;
    self->handle_event = handle_closed;
}

/* Public functions */
StdRet_t Sm_Init (SmType *self)
{
    assert(self != NULL);

    StdRet_t ret = OK;

    set_initial_values(self);
    
    self->handle_event = handle_closed; /* Initial state handler */

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);

    return ret;
}

void Sm_HandleEvent(SmType *self, const Event event, const PDU_S recv_pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);
    /* Delegate the event handling to the appropriate state handler */
    self->handle_event(self, event, recv_pdu);

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);

    /* Additional event handling for timer events */
    if (event == EVENT_TH_ELAPSED) {
        switch (self->state) {
            case STATE_START:
                pdu_to_send = HB(*self);
                /* TODO: RTR - Send HB */
                break;
            case STATE_UP:
                pdu_to_send = HB(*self);
                /* TODO: RTR - Send HB */
                break;
            case STATE_RETR_REQ:
                pdu_to_send = HB(*self);
                /* TODO: RTR - Send HB */
                break;
            case STATE_RETR_RUN:
                pdu_to_send = HB(*self);
                /* TODO: RTR - Send HB */
                break;
            default:
                break;
        }
    } else if (event == EVENT_TI_ELAPSED) {
        close_connection(self);

        pdu_to_send = DiscReq(2, NO_DETAILED_REASON, *self);
        /* TODO: RTR - Send DiscReq(4) */
    }

    /* Update state handler */
    switch (self->state) {
        case STATE_CLOSED:
            self->handle_event = handle_closed;
            break;
        case STATE_DOWN:
            self->handle_event = handle_down;
            break;
        case STATE_START:
            self->handle_event = handle_start;
            break;
        case STATE_UP:
            self->handle_event = handle_up;
            break;
        case STATE_RETR_REQ:
            self->handle_event = handle_retr_req;
            break;
        case STATE_RETR_RUN:
            self->handle_event = handle_retr_run;
            break;
        default:
            /* Should never reach this point if all states are handled */
            assert(0);
            break;
    }
}
