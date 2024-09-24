#include "sm.h"
#include "assert.h"
#include "log.h"
#include "rass.h"

static uint8_t buff_to_send[MAX_BUFF_SIZE] = {0};

/* Private function prototypes */
static void set_initial_values(SmType *self);
static void close_connection(SmType *self, const PDU_S *pdu);
static void process_regular_receipt(SmType *self, const PDU_S *pdu);
static void handle_closed(SmType *self, const Event event, PDU_S *pdu);
static void handle_down(SmType *self, const Event event, PDU_S *pdu);
static void handle_start(SmType *self, const Event event, PDU_S *pdu);
static void handle_up(SmType *self, const Event event, PDU_S *pdu);
static void handle_retr_req(SmType *self, const Event event, PDU_S *pdu);
static void handle_retr_run(SmType *self, const Event event, PDU_S *pdu);
static bool check_seq_confirmed_timestamp(SmType *self, const PDU_S *pdu);
static bool check_version(const PDU_S *pdu);

static bool check_seq_confirmed_timestamp(SmType *self, const PDU_S *pdu)
{
    assert(self != NULL);
    assert(pdu != NULL);

    bool ret = false;

    if(((pdu->confirmed_timestamp - self->ctsr) >= 0) && ((pdu->confirmed_timestamp - self->ctsr) < self->time.timeouts.Tmax))
    {
        ret=true;
    }
    else
    {
        ret=false;
    }

    return ret;
}

static bool check_version(const PDU_S *pdu)
{
    assert(pdu->payload != NULL);

    bool ret = false;
    
    if ((pdu->payload[0] == ((PROTOCOL_VERSION >> SHIFT_3_BYTES) & 0xFF)) &&
        (pdu->payload[1] == ((PROTOCOL_VERSION >> SHIFT_2_BYTES) & 0xFF)) &&
        (pdu->payload[2] == ((PROTOCOL_VERSION >> SHIFT_1_BYTES) & 0xFF)) &&
        (pdu->payload[3] == (PROTOCOL_VERSION & 0xFF)))
    {
        ret =  true;
    }
    else
    {
        ret =  false;
    }

    return ret;
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
    self->cst = 0;
    self->csr = 0;
    self->tsr = 0;
    self->ctsr = 0;
}

static void close_connection(SmType *self, const PDU_S *pdu)
{
    assert(self != NULL);
    self->cst = pdu->sequence_number;
    self->state = STATE_CLOSED;
    self->handle_event = handle_closed;
}

static void process_regular_receipt(SmType *self, const PDU_S *pdu)
{
    assert(self != NULL);
    assert(pdu != NULL);

    self->snr = pdu->sequence_number + 1;
    self->cst = pdu->sequence_number;
    self->csr = pdu->confirmed_sequence_number;
    self->tsr = pdu->timestamp;
    self->ctsr = pdu->confirmed_timestamp;
}

/* Generates pseudo-random number between 0 and 100 */
int snt_rand_value()
{
    static unsigned int seed = 12345;
    seed = (seed * 1103515245 + 12345) % 101;

    return seed;
}

static void handle_closed(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

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
                self->ctsr = self->time.Tlocal();
                self->state = STATE_START;

                /* Send ConnReq */
                ConnReq(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            break;
        default:
            /* No action for other events in STATE_CLOSED */
            break;
    }
}

static void handle_down(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);
    
    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_CLOSE_CONN:
        case EVENT_SEND_DATA:
            close_connection(self, pdu);
            break;

        case EVENT_RECV_CONN_REQ:
            if (check_version(pdu)) 
            {
                process_regular_receipt(self, pdu);
                self->csr = self->snt - 1;
                self->ctsr = self->time.Tlocal();
                self->state = STATE_START;

                self->time.Trtd = self->time.Tlocal() - self->ctsr;
                self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;

                /* Send ConnResp */
                ConnResp(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            else
            {
                close_connection(self, pdu);

                /* Send DiscReq(6) */
                DiscReq(self, pdu, PROTOCOL_VERSION_ERROR, NO_DETAILED_REASON);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            break;
        default:
            /* No action for other events in STATE_DOWN */
            break;
    }
}

static void handle_start(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

    switch (event) {
        case EVENT_OPEN_CONN:
        case EVENT_SEND_DATA:
            close_connection(self, pdu);

            /* Send DiscReq(5) */
            DiscReq(self, pdu, STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_RETR_REQ:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_DATA:
        case EVENT_RECV_RETR_DATA:
            close_connection(self, pdu);

            /* Send DiscReq(2) */
            DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(0) */
            DiscReq(self, pdu, USER_REQUEST, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;


        case EVENT_RECV_CONN_RESP:
            if (self->role == ROLE_SERVER)
            {
                close_connection(self, pdu);

                /* Send DiscReq(2) */
                DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            else if (self->role == ROLE_CLIENT)
            {
                if (check_version(pdu)) 
                {
                    process_regular_receipt(self, pdu);
                    self->state = STATE_UP;

                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;

                    /* Send HB */
                    HB(self, pdu);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
                else
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(6) */
                    DiscReq(self, pdu, PROTOCOL_VERSION_ERROR, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            break;

            case EVENT_RECV_DISC_REQ:
                close_connection(self, pdu);
            break;

            case EVENT_RECV_HB:
                if (self->role == ROLE_SERVER)
                {
                    /* Checking the sequence number SNinSeq == true */
                    if (self->snr == pdu->sequence_number)
                    {
                        /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                        if (check_seq_confirmed_timestamp(self, pdu))
                        {
                            process_regular_receipt(self, pdu);
                            self->state = STATE_UP;

                            self->time.Trtd = self->time.Tlocal() - self->ctsr;
                            self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                        }
                        else
                        {
                            close_connection(self, pdu);

                            /* Send DiscReq(8) */
                            DiscReq(self, pdu, SEQ_ERR, NO_DETAILED_REASON);
                            serialize_pdu(pdu, buff_to_send, pdu->message_length);
                            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                        }
                    }
                    else 
                    {
                        close_connection(self, pdu);

                        /* Send DiscReq(3) */
                        DiscReq(self, pdu, SEQ_NBR_ERR_FOR_CONNECTION, NO_DETAILED_REASON);
                        serialize_pdu(pdu, buff_to_send, pdu->message_length);
                        self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                    }
                }
                else if (self->role == ROLE_CLIENT)
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(2) */
                    DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            break;

        default:
            /* No action for other events in STATE_START */
            break;
    }
}

static void handle_up(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(5) */
            DiscReq(self, pdu, STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(0) */
            DiscReq(self, pdu, USER_REQUEST, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_SEND_DATA:
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
        case EVENT_RECV_RETR_DATA:
            close_connection(self, pdu);

            /* Send DiscReq(2) */
            DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self, pdu);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    process_regular_receipt(self, pdu);
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */

                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                } 
                else 
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(7) */
                    DiscReq(self, pdu, FAIL_RETRANSMISSION, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            else
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = pdu->confirmed_sequence_number;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                }
                else 
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(7) */
                    DiscReq(self, pdu, FAIL_RETRANSMISSION, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            break;

        case EVENT_RECV_HB:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self, pdu))
                {
                    process_regular_receipt(self, pdu);
                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                }
                else
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(8) */
                    DiscReq(self, pdu, SEQ_ERR, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                /* Send RetrReq */
                RetrReq(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            break;

        case EVENT_RECV_DATA:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self, pdu))
                {
                    process_regular_receipt(self, pdu);
                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                }
                else
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(8) */
                    DiscReq(self, pdu, SEQ_ERR, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;
                
                /* Send RetrReq */
                RetrReq(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            break;
        default:
            /* No action for other events in STATE_UP */
            break;
    }
}

static void handle_retr_req(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(5) */
            DiscReq(self, pdu, STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(0) */
            DiscReq(self, pdu, USER_REQUEST, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
            close_connection(self, pdu);

            /* Send DiscReq(2) */
            DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self, pdu);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    process_regular_receipt(self, pdu);
                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                } 
                else 
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(7) */
                    DiscReq(self, pdu, FAIL_RETRANSMISSION, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            else
            {
                if (unconfirmed_payload_available()) 
                {
                    self->csr = pdu->confirmed_sequence_number;
                    /* TODO: RTR - Send RetrResp, RetrData(s), HB or Data */
                }
                else 
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(7) */
                    DiscReq(self, pdu, FAIL_RETRANSMISSION, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            break;

        case EVENT_SEND_DATA:
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_RETR_RESP:
            self->state = STATE_RETR_RUN;
            break;

        default:
            /* No action for other events in STATE_RETR_REQ */
            break;
    }
}

static void handle_retr_run(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

    switch (event) {
        case EVENT_OPEN_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(5) */
            DiscReq(self, pdu, STATE_SERVICE_NOT_ALLOWED, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_CLOSE_CONN:
            close_connection(self, pdu);

            /* Send DiscReq(0) */
            DiscReq(self, pdu, USER_REQUEST, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_SEND_DATA:
            /* Send Data */
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_CONN_REQ:
        case EVENT_RECV_CONN_RESP:
        case EVENT_RECV_RETR_RESP:
            close_connection(self, pdu);

            /* Send DiscReq(2) */
            DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
            serialize_pdu(pdu, buff_to_send, pdu->message_length);
            self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            break;

        case EVENT_RECV_DISC_REQ:
            close_connection(self, pdu);
            break;

        case EVENT_RECV_RETR_REQ:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                close_connection(self, pdu);

                /* Send DiscReq(2) */
                DiscReq(self, pdu, NOT_EXPECTED_RECV_MSG_TYPE, NO_DETAILED_REASON);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            else
            {
                /* Verify all unconfirmed payload data available */
                if (unconfirmed_payload_available()) 
                {
                    self->csr = pdu->confirmed_sequence_number;
                    self->state = STATE_RETR_REQ;
                    /* TODO: RTR - Send RetrResp RetrData(s) HB or Data RetrReq */
                }
                else
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(7) */
                    DiscReq(self, pdu, FAIL_RETRANSMISSION, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            break;

        case EVENT_RECV_HB:
        case EVENT_RECV_DATA:
            /* Checking the sequence number SNinSeq == true */
            if (self->snr == pdu->sequence_number)
            {
                /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                if (check_seq_confirmed_timestamp(self, pdu))
                {
                    process_regular_receipt(self, pdu);
                    self->state = STATE_UP;

                    self->time.Trtd = self->time.Tlocal() - self->ctsr;
                    self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                } 
                else
                {
                    close_connection(self, pdu);

                    /* Send DiscReq(8) */
                    DiscReq(self, pdu, SEQ_ERR, NO_DETAILED_REASON);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            }
            else
            {
                self->state = STATE_RETR_REQ;

                /* Send RetrReq */
                RetrReq(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
            }
            break;

            case EVENT_RECV_RETR_DATA:
                /* Checking the sequence number SNinSeq == true */
                if (self->snr == pdu->sequence_number)
                {
                    /* Checking the Sequence of the Confirmed Timestamps CTSinSeq == true */
                    if (check_seq_confirmed_timestamp(self, pdu))
                    {
                        process_regular_receipt(self, pdu);
                        self->state = STATE_RETR_RUN;

                        self->time.Trtd = self->time.Tlocal() - self->ctsr;
                        self->time.Ti = self->time.timeouts.Tmax - self->time.Trtd;
                    } 
                    else
                    {
                        close_connection(self, pdu);

                        /* Send DiscReq(8) */
                        DiscReq(self, pdu, SEQ_ERR, NO_DETAILED_REASON);
                        serialize_pdu(pdu, buff_to_send, pdu->message_length);
                        self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                    }
                }
                else
                {
                    self->state = STATE_RETR_REQ;

                    /* Send RetrReq */
                    RetrReq(self, pdu);
                    serialize_pdu(pdu, buff_to_send, pdu->message_length);
                    self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                }
            break;

        default:
            /* No action for other events in STATE_RETR_RUN */
            break;
    }
}

/* Public functions */
StdRet_t Sm_Init(SmType *self)
{
    assert(self != NULL);

    StdRet_t ret = OK;

    /* TODO: RTR - Config timeouts*/
    self->time.timeouts.Th = 10;    /* Th = 10 sec */
    self->time.timeouts.Tmax = 30;  /* Tmax = 30 sec */
    self->time.Ti = self->time.timeouts.Tmax; /* Initially Ti = Tmax */

    set_initial_values(self);
    
    self->handle_event = handle_closed; /* Initial state handler */

    LOG_INFO("connection: %i, state: %i", self->channel, self->state);

    return ret;
}

void Sm_HandleEvent(SmType *self, const Event event, PDU_S *pdu)
{
    assert(self != NULL);

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
                HB(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                break;

            case STATE_UP:
                /* Send HB */
                HB(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                break;

            case STATE_RETR_REQ:
                /* Send HB */
                HB(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                break;

            case STATE_RETR_RUN:
                /* Send HB */
                HB(self, pdu);
                serialize_pdu(pdu, buff_to_send, pdu->message_length);
                self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
                break;

            default:
                break;
        }
    } else if (event == EVENT_TI_ELAPSED) {
        close_connection(self, pdu);

        /* Send DiscReq(4) */
        DiscReq(self, pdu, TIMEOUT_INCOMING_MSG, NO_DETAILED_REASON);
        serialize_pdu(pdu, buff_to_send, pdu->message_length);
        self->vtable->SendSpdu(self->channel, pdu->message_length, buff_to_send);
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
