#include "sm.h"
#include "assert.h"
#include "log.h"
#include "rass.h"

extern SafeComVtable globalVtable;

/* Private function prototypes */
static void set_initial_values(SmType *self);
static void close_connection(SmType *self);
static void process_regular_receipt(SmType *self);
static void handle_closed(SmType *self, const Event event, const PDU_S *pdu);
static void handle_down(SmType *self, const Event event, const PDU_S *pdu);
static void handle_start(SmType *self, const Event event, const PDU_S *pdu);
static void handle_up(SmType *self, const Event event, const PDU_S *pdu);
static void handle_retr_req(SmType *self, const Event event, const PDU_S *pdu);
static void handle_retr_run(SmType *self, const Event event, const PDU_S *pdu);
static bool check_seq_confirmed_timestamp(SmType *self);
static bool check_version(const PDU_S *recv_pdu);

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

static bool check_version(const PDU_S *recv_pdu)
{
    assert(recv_pdu->payload != NULL);
    
    if ((recv_pdu->payload[0] == ((PROTOCOL_VERSION >> SHIFT_3_BYTES) & 0xFF)) &&
        (recv_pdu->payload[1] == ((PROTOCOL_VERSION >> SHIFT_2_BYTES) & 0xFF)) &&
        (recv_pdu->payload[2] == ((PROTOCOL_VERSION >> SHIFT_1_BYTES) & 0xFF)) &&
        (recv_pdu->payload[3] == (PROTOCOL_VERSION & 0xFF)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool unconfirmed_payload_available()
{
    /* TODO: RTR - all unconfirmed payload data available */
    return false;
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

/* Generates pseudo-random number between 0 and 100 */
int snt_rand_value()
{
    static unsigned int seed = 12345;
    seed = (seed * 1103515245 + 12345) % 101;

    return seed;
}

static void handle_closed(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);
    UNUSED(pdu);

    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    switch (event) {
        case EVENT_OPEN_CONN:
            if(self->role == ROLE_SERVER)
            {
                self->snt = snt_rand_value(); /* Random value for SNT */
                self->state = STATE_DOWN;
            }
            else if(self->role == ROLE_CLIENT)
            {
                self->snt = snt_rand_value(); /* Random value for SNT */
                self->cst = 0;
                self->ctsr = 0; /* TODO: RTR - Tlocal */

                /* Send ConnReq */
                pdu_to_send = ConnReq(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);

                self->state = STATE_START;
            }
            break;
        default:
            /* No action for other events in STATE_CLOSED */
            break;
    }
}

static void handle_down(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};
    
    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_CLOSE_CONN:
        case EVENT_SEND_DATA:
            close_connection(self);
            break;

        case EVENT_RECV_CONN_REQ:
            if (check_version(pdu)) 
            {
                self->csr = self->snt - 1;
                self->ctsr = 0; /* TODO: RTR - Tlocal */
                self->state = STATE_START;

                /* Send ConnResp */
                pdu_to_send = ConnResp(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            }
            else
            {
                close_connection(self);

                /* Send DiscReq(6) */
                pdu_to_send = DiscReq(PROTOCOL_VERSION_ERROR, NO_DETAILED_REASON, self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            }
            break;
        default:
            /* No action for other events in STATE_DOWN */
            break;
    }
}

static void handle_start(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_SEND_DATA:
            close_connection(self);

            /* Send DiscReq(5) */
            pdu_to_send = DiscReq(STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_RETR_REQ:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_DATA:
        case EVENT_RECV_RETR_DATA:
            close_connection(self);

            /* Send DiscReq(2) */
            pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            /* Send DiscReq(0) */
            pdu_to_send = DiscReq(USER_REQUEST, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;


        case EVENT_RECV_CONN_RESP:
            if (self->role == ROLE_SERVER)
            {
                close_connection(self);

                /* Send DiscReq(2) */
                pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            }
            else if (self->role == ROLE_CLIENT)
            {
                if (check_version(pdu)) 
                {
                    self->state = STATE_UP;

                    /* Send HB */
                    pdu_to_send = HB(self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
                else
                {
                    close_connection(self);

                    /* Send DiscReq(6) */
                    pdu_to_send = DiscReq(PROTOCOL_VERSION_ERROR, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

                            /* Send DiscReq(8) */
                            pdu_to_send = DiscReq(SEQ_ERR, NO_DETAILED_REASON, self);
                            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                        }
                    }
                    else 
                    {
                        close_connection(self);

                        /* Send DiscReq(3) */
                        pdu_to_send = DiscReq(SEQ_NBR_ERR_FOR_CONNECTION, NO_DETAILED_REASON, self);
                        serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                        globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                    }
                }
                else if (self->role == ROLE_CLIENT)
                {
                    close_connection(self);

                    /* Send DiscReq(2) */
                    pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            break;

        default:
            /* No action for other events in STATE_START */
            break;
    }
}

static void handle_up(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);
    
    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            /* Send DiscReq(5) */
            pdu_to_send = DiscReq(STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            /* Send DiscReq(0) */
            pdu_to_send = DiscReq(USER_REQUEST, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_SEND_DATA:
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            globalVtable.SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_RETR_DATA:
            close_connection(self);

            /* Send DiscReq(2) */
            pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                } 
                else 
                {
                    close_connection(self);

                    /* Send DiscReq(7) */
                    pdu_to_send = DiscReq(FAIL_RETRANSMISSION, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            }
            else
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = self->cspdu;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                }
                else 
                {
                    close_connection(self);

                    /* Send DiscReq(7) */
                    pdu_to_send = DiscReq(FAIL_RETRANSMISSION, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

                    /* Send DiscReq(8) */
                    pdu_to_send = DiscReq(SEQ_ERR, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                /* Send RetrReq */
                pdu_to_send = RetrReq(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

                    /* Send DiscReq(8) */
                    pdu_to_send = DiscReq(SEQ_ERR, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;
                
                /* Send RetrReq */
                pdu_to_send = RetrReq(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            }
            break;
        default:
            /* No action for other events in STATE_UP */
            break;
    }
}

static void handle_retr_req(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);
    UNUSED(pdu);
    
    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            /* Send DiscReq(5) */
            pdu_to_send = DiscReq(STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            /* Send DiscReq(0) */
            pdu_to_send = DiscReq(USER_REQUEST, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
            close_connection(self);

            /* Send DiscReq(2) */
            pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                } 
                else 
                {
                    close_connection(self);

                    /* Send DiscReq(7) */
                    pdu_to_send = DiscReq(FAIL_RETRANSMISSION, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

                    /* Send DiscReq(7) */
                    pdu_to_send = DiscReq(FAIL_RETRANSMISSION, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            }
            break;

        case EVENT_SEND_DATA:
            self->state = STATE_RETR_REQ;
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            globalVtable.SendSpdu(self->channel, pdu->message_length, buff_to_send);
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

static void handle_retr_run(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);
    UNUSED(pdu);
    
    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self);

            /* Send DiscReq(5) */
            pdu_to_send = DiscReq(STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self);

            /* Send DiscReq(0) */
            pdu_to_send = DiscReq(USER_REQUEST, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_SEND_DATA:
            self->state = STATE_RETR_RUN;
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            globalVtable.SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
            close_connection(self);

            /* Send DiscReq(2) */
            pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
            serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
            globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == self->snpdu)
            {
                close_connection(self);

                /* Send DiscReq(2) */
                pdu_to_send = DiscReq(NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON, self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
            }
            else
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = self->cspdu;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp RetrData(s) HB or Data RetrReq */
                }
                else
                {
                    close_connection(self);

                    /* Send DiscReq(7) */
                    pdu_to_send = DiscReq(FAIL_RETRANSMISSION, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

                    /* Send DiscReq(8) */
                    pdu_to_send = DiscReq(SEQ_ERR, NO_DETAILED_REASON, self);
                    serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                    globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                /* Send RetrReq */
                pdu_to_send = RetrReq(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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

void Sm_HandleEvent(SmType *self, const Event event, const PDU_S *pdu)
{
    assert(self != NULL);

    PDU_S pdu_to_send = { 0 };
    uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);
    /* Delegate the event handling to the appropriate state handler */
    self->handle_event(self, event, pdu);

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);

    /* Additional event handling for timer events */
    if (event == EVENT_TH_ELAPSED) 
    {
        switch (self->state) 
        {
            case STATE_START:
                /* Send HB */
                pdu_to_send = HB(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                break;

            case STATE_UP:
                /* Send HB */
                pdu_to_send = HB(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                break;

            case STATE_RETR_REQ:
                /* Send HB */
                pdu_to_send = HB(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                break;

            case STATE_RETR_RUN:
                /* Send HB */
                pdu_to_send = HB(self);
                serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
                globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
                break;

            default:
                break;
        }
    } else if (event == EVENT_TI_ELAPSED) {
        close_connection(self);

        /* Send DiscReq(4) */
        pdu_to_send = DiscReq(TIMEOUT_INCOMING_MSG, NO_DETAILED_REASON, self);
        serialize_pdu(&pdu_to_send, buff_to_send, pdu_to_send.message_length);
        globalVtable.SendSpdu(self->channel, pdu_to_send.message_length, buff_to_send);
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
