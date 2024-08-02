#include "pdu.h"
#include "md4.h"
#include "assert.h"

static void write_uint16(uint8_t *buffer, size_t *offset, uint16_t value)
 {
    assert(buffer != NULL);
    assert(offset != NULL);

    buffer[*offset] = (uint8_t)(value >> SHIFT_1_BYTES);
    buffer[*offset + 1] = (uint8_t)value;
    *offset += 2;
}

static void write_uint32(uint8_t *buffer, size_t *offset, uint32_t value) 
{
    assert(buffer != NULL);
    assert(offset != NULL);

    buffer[*offset] = (uint8_t)(value >> SHIFT_3_BYTES);
    buffer[*offset + 1] = (uint8_t)(value >> SHIFT_2_BYTES);
    buffer[*offset + 2] = (uint8_t)(value >> SHIFT_1_BYTES);
    buffer[*offset + 3] = (uint8_t)value;
    *offset += 4;
}

static uint16_t read_uint16(const uint8_t *buffer, size_t *offset) 
{
    assert(buffer != NULL);
    assert(offset != NULL);

    uint16_t value = (uint16_t)(buffer[*offset] << SHIFT_1_BYTES) | (uint16_t)buffer[*offset + 1];
    *offset += 2;
    return value;
}

static uint32_t read_uint32(const uint8_t *buffer, size_t *offset) 
{
    assert(buffer != NULL);
    assert(offset != NULL);

    uint32_t value = (uint32_t)(buffer[*offset] << SHIFT_3_BYTES) | (uint32_t)(buffer[*offset + 1] << SHIFT_2_BYTES) |
                     (uint32_t)(buffer[*offset + 2] << SHIFT_1_BYTES) | (uint32_t)buffer[*offset + 3];
    *offset += 4;
    return value;
}

/* Create payload for Connection Request */
static uint8_t *ConnReqPayload()
{
    static uint8_t buffer[CONN_REQ_PAYLOAD_LENGTH];

    buffer[0] = (PROTOCOL_VERSION >> SHIFT_3_BYTES) & 0xFF;
    buffer[1] = (PROTOCOL_VERSION >> SHIFT_2_BYTES ) & 0xFF;
    buffer[2] = (PROTOCOL_VERSION >> SHIFT_1_BYTES) & 0xFF;
    buffer[3] = PROTOCOL_VERSION & 0xFF;

    /* TODO: RTR - Byte 4 - 5 is for Nsendmax: Size of the local receive buffer in number of messages */
    buffer[4] = 0;
    buffer[5] = 0;

    /* Byte 6 - 13 are reserved for initial parameter adjustment (not used yet) with value = 0 */
    for (uint8_t i = 6; i < CONN_REQ_PAYLOAD_LENGTH; i++)
    {
        buffer[i] = 0;
    }
    
    return buffer;
}

/* Create payload for Connection Response */
static uint8_t *ConnRespPayload()
{
    static uint8_t buffer[CONN_RESP_PAYLOAD_LENGTH];

    buffer[0] = (PROTOCOL_VERSION >> SHIFT_3_BYTES) & 0xFF;
    buffer[1] = (PROTOCOL_VERSION >> SHIFT_2_BYTES ) & 0xFF;
    buffer[2] = (PROTOCOL_VERSION >> SHIFT_1_BYTES) & 0xFF;
    buffer[3] = PROTOCOL_VERSION & 0xFF;

    /* TODO: RTR - Byte 4 - 5 is for Nsendmax: Size of the local receive buffer in number of messages */
    buffer[4] = 0;
    buffer[5] = 0;

    /* Byte 6 - 13 are reserved for initial parameter adjustment (not used yet) with value = 0 */
    for (uint8_t i = 6; i < CONN_REQ_PAYLOAD_LENGTH; i++)
    {
        buffer[i] = 0;
    }
    
    return buffer;
}

/* Create payload for Retransmission Request */
static uint8_t *RetrReqPayload()
{
    /* RetrReq has no payload */
    return NULL;
}

/* Create payload for Retransmission Response */
static uint8_t *RetrRespPayload()
{
    /* RetrReq has no payload */
    return NULL;
}

/* Create payload for Disconnection Request */
static uint8_t *DiscReqPayload(DiscReasonType discReason)
{
    static uint8_t buffer[DISC_REQ_PAYLOAD_LENGTH];

    /* TODO: RTR - Byte 0 - 1 are for detailed information regarding the reason for the disconnection request. */
    buffer[0] = 0;
    buffer[1] = 0;
    /* Byte 2 - 3 are for disconnection reason */
    buffer[2] = 0;
    buffer[3] = discReason;

    return buffer;
}

/* Create payload for Heartbeat */
uint8_t *HBPayload()
{
    /* HB has no payload data */
    return NULL;
}

/* Calculate security code of a RaSTA telegram */
static void calculate_MD4(PDU_S *pdu)
{
    assert(pdu != NULL);
    
	MD4_CTX ctx;

    uint8_t buffer[MAX_PDU_LENGTH];

    uint8_t safety_code[16];

    /* Serialize data without safety code */
    serialize_pdu(pdu, buffer, pdu->message_length);

    MD4_Init(&ctx);
	MD4_Update(&ctx, buffer, pdu->message_length - SAFETY_CODE_LENGTH);
    MD4_Final(safety_code, &ctx);

    pdu->safety_code = safety_code;
}

/* Serialize fields in to a buffer with data from PDU structure */
void serialize_pdu(PDU_S *pdu, uint8_t *buffer, const size_t buffer_size) 
{
    assert(pdu != NULL);
    assert(buffer != NULL);
    
    if (buffer_size < pdu->message_length) {
        return;
    }

    size_t offset = 0;

    /* Serialize fixed fields */
    write_uint16(buffer, &offset, pdu->message_length);
    write_uint16(buffer, &offset, pdu->message_type);
    write_uint32(buffer, &offset, pdu->receiver_id);
    write_uint32(buffer, &offset, pdu->sender_id);
    write_uint32(buffer, &offset, pdu->sequence_number);
    write_uint32(buffer, &offset, pdu->confirmed_sequence_number);
    write_uint32(buffer, &offset, pdu->timestamp);
    write_uint32(buffer, &offset, pdu->confirmed_timestamp);

    /* Serialize payload */
    if (pdu->payload != NULL) {
        size_t payload_length = pdu->message_length - PDU_FIXED_FIELDS_LENGTH;
        assert(payload_length >= 0);  /* Ensure payload length is valid */
        for (size_t i = 0; i < payload_length; ++i) {
            buffer[offset++] = pdu->payload[i];
        }
    }

    /* Serialize safety code */
    if (pdu->safety_code != NULL) {
        for (size_t i = 0; i < SAFETY_CODE_LENGTH; ++i) {
            buffer[offset++] = pdu->safety_code[i];
        }
    }
}

/* Deserialize data in to the PDU structure from a buffer with serialized data */
void deserialize_pdu(uint8_t *buffer, const size_t buffer_size, PDU_S *pdu) 
{
    assert(buffer != NULL);
    assert(pdu != NULL);

    if (buffer_size < PDU_FIXED_FIELDS_LENGTH) {
        return;
    }

    size_t offset = 0;

    /* Deserialize fixed fields */
    pdu->message_length = read_uint16(buffer, &offset);
    pdu->message_type = (MessageType)read_uint16(buffer, &offset);
    pdu->receiver_id = read_uint32(buffer, &offset);
    pdu->sender_id = read_uint32(buffer, &offset);
    pdu->sequence_number = read_uint32(buffer, &offset);
    pdu->confirmed_sequence_number = read_uint32(buffer, &offset);
    pdu->timestamp = read_uint32(buffer, &offset);
    pdu->confirmed_timestamp = read_uint32(buffer, &offset);

    /* Check if buffer size is sufficient for the full PDU */
    if (buffer_size < pdu->message_length) {
        return;
    }

    /* Deserialize payload */
    size_t payload_length = pdu->message_length - PDU_FIXED_FIELDS_LENGTH;
    assert(payload_length >= 0);  /* Ensure payload length is valid */
    pdu->payload = buffer + offset;
    offset += payload_length;

    /* Deserialize safety code */
    pdu->safety_code = buffer + offset;
}

/* Create PDU for Connection Request */
void ConnReq(PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 50;
    ret_pdu->message_type = CONNECTION_REQUEST;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = ConnReqPayload();

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}

/* Create PDU for Connection Response */
void ConnResp(PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 50;
    ret_pdu->message_type = CONNECTION_RESPONSE;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = ConnRespPayload();

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}

/* Create PDU for Retransmission Request */
void RetrReq(PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 36;
    ret_pdu->message_type = RETRANSMISSION_REQUEST;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = RetrReqPayload();

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}

/* Create PDU for Retransmission Response */
void RetrResp(PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 36;
    ret_pdu->message_type = RETRANSMISSION_RESPONSE;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = RetrRespPayload();

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}

/* Create PDU for Disconnection Request */
void DiscReq(DiscReasonType discReason, PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 40;
    ret_pdu->message_type = DISCONNECTION_REQUEST;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = DiscReqPayload(discReason);

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}

/* Create PDU for Heartbeat */
void HB(PDU_S *ret_pdu)
{
    static uint8_t serialized_pdu[MAX_PDU_LENGTH];

    ret_pdu->message_length = 36;
    ret_pdu->message_type = HEARTBEAT;
    ret_pdu->receiver_id = RECEIVER_ID;
    ret_pdu->sender_id = SENDER_ID;
    ret_pdu->sequence_number = 0;
    ret_pdu->confirmed_sequence_number = 0;
    ret_pdu->timestamp = 0;
    ret_pdu->confirmed_timestamp = 0;
    ret_pdu->payload = HBPayload();

    /* Calculate the safety code with MD4 protocol */
    calculate_MD4(ret_pdu);
}