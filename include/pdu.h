#ifndef PDU_H
#define PDU_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAFETY_CODE_LENGTH          8
#define PDU_FIXED_FIELDS_LENGTH     36
#define MAX_PDU_LENGTH              50
#define CONN_REQ_PAYLOAD_LENGTH     14
#define CONN_RESP_PAYLOAD_LENGTH    14
#define DISC_REQ_PAYLOAD_LENGTH     4

#define SENDER_ID   0x00000000
#define RECEIVER_ID 0x00000000

#define PROTOCOL_VERSION    0x30333031 /* Protocol version, for which all 4 bytes are decimal digits in ASCII, 
                                        e. g.: “0301” = 0x30 0x33 0x30 0x31 = Version 03.01 */

#define SHIFT_1_BYTES  8
#define SHIFT_2_BYTES  16
#define SHIFT_3_BYTES  24

/* Enum for message types */
typedef enum {
    CONNECTION_REQUEST = 6200,
    CONNECTION_RESPONSE = 6201,
    RETRANSMISSION_REQUEST = 6212,
    RETRANSMISSION_RESPONSE = 6213,
    DISCONNECTION_REQUEST = 6216,
    HEARTBEAT = 6220,
    DATA = 6240,
    RETRANSMITTED_DATA = 6241
} MessageType;

/* PDU structure */
typedef struct {
    uint16_t message_length;
    MessageType message_type;
    uint32_t receiver_id;
    uint32_t sender_id;
    uint32_t sequence_number;
    uint32_t confirmed_sequence_number;
    uint32_t timestamp;
    uint32_t confirmed_timestamp;
    uint8_t *payload;
    uint8_t *safety_code;
} PDU_S;

typedef enum {
    USER_REQUEST,
    UNDEFINED_MSG_TYPE_RECV,
    NOT_EXPECTED_RECV_MSG_TYPE,
    SEQ_NBR_ERR_FOR_CONNECTION,
    TIMEOUT_INCOMING_MSG,
    STATE_SERVICE_NOT_ALLOWED,
    PROTOCOL_VERSION_ERROR,
    FAIL_RETRANSMISSION,
    SEQ_ERR
} DiscReasonType;

/**
 * @brief Serialize fields in to a buffer with data from PDU structure.
 *
 * @param[in]   pdu         Protocol Data Unit (PDU_S) structure.
 * @param[out]  buffer      Buffer that will be serialized with PDU_S structure.
 * @param[in]   buffer_size The size of the buffer (PDU_FIXED_FIELDS_LENGTH + payload length). Payload length depends on message type.
 */
void serialize_pdu(PDU_S *pdu, uint8_t *buffer, const size_t buffer_size);

/**
 * @brief Deserialize data in to the PDU structure from a buffer with serialized data.
 *
 * @param[in]   buffer      Buffer that will be deserialized in to the PDU_S structure.
 * @param[in]   buffer_size The size of the buffer (PDU_FIXED_FIELDS_LENGTH + payload length). Payload length depends on message type. 
 * @param[out]  pdu         Protocol Data Unit (PDU_S) structure.
 */
void deserialize_pdu(uint8_t *buffer, const size_t buffer_size, PDU_S *pdu);

/**
 * @brief Create PDU for Connection Request.
 * 
 * @param[out]  ret_pdu     The returned Connection Request PDU.
 */
void ConnReq(PDU_S *ret_pdu);

/**
 * @brief Create PDU for Connection Response.
 * 
 * @param[out]  ret_pdu     The returned Connection Response PDU.
 */
void ConnResp(PDU_S *ret_pdu);

/**
 * @brief Create PDU for Retransmission Request.
 * 
 * @param[out]  ret_pdu     The returned Retransmission Request PDU.
 */
void RetrReq(PDU_S *ret_pdu);

/**
 * @brief Create PDU for Retransmission Response.
 * 
 * @param[out]  ret_pdu     The returned Retransmission Response PDU.
 */
void RetrResp(PDU_S *ret_pdu);

/**
 * @brief Create PDU for Disconnection Request.
 * 
 * @param[in]   discReason  Disconnection reason.
 * @param[out]  ret_pdu     The returned Disconnection Request PDU.
 */
void DiscReq(DiscReasonType discReason, PDU_S *ret_pdu);


/**
 * @brief Create PDU for Heartbeat.
  * 
 * @param[out]  ret_pdu     The returned Heartbeat PDU.
 */
void HB(PDU_S *ret_pdu);

#endif // PDU_H
