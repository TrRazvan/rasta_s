#ifndef PDU_H
#define PDU_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md4.h"

#define SAFETY_CODE_LENGTH          8U
#define PDU_FIXED_FIELDS_LENGTH     36U
#define MAX_PDU_LENGTH              50U
#define CONN_REQ_PAYLOAD_LENGTH     14U
#define CONN_RESP_PAYLOAD_LENGTH    14U
#define DISC_REQ_PAYLOAD_LENGTH     4U

#define SENDER_ID   0x00000000U
#define RECEIVER_ID 0x00000000U

#define PROTOCOL_VERSION    0x30333031U /* Protocol version, for which all 4 bytes are decimal digits in ASCII, 
                                        e. g.: “0301” = 0x30 0x33 0x30 0x31 = Version 03.01 */

#define N_SEND_MAX  0xFFFFU

#define NO_DETAILED_REASON 0x0000U

#define SHIFT_1_BYTES  8U
#define SHIFT_2_BYTES  16U
#define SHIFT_3_BYTES  24U

/* State machine context structure */
typedef struct SmType SmType;

/* Enum for message types */
typedef enum {
    CONNECTION_REQUEST = 6200U,
    CONNECTION_RESPONSE = 6201U,
    RETRANSMISSION_REQUEST = 6212U,
    RETRANSMISSION_RESPONSE = 6213U,
    DISCONNECTION_REQUEST = 6216U,
    HEARTBEAT = 6220U,
    DATA = 6240U,
    RETRANSMITTED_DATA = 6241U
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
    const uint8_t *payload;
    uint8_t *safety_code;
} PDU_S;

typedef enum {
    USER_REQUEST = 0U,
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
void serialize_pdu(const PDU_S *pdu, uint8_t *buffer, const size_t buffer_size);

/**
 * @brief Deserialize data in to the PDU structure from a buffer with serialized data.
 *
 * @param[in]   buffer      Buffer that will be deserialized in to the PDU_S structure.
 * @param[in]   buffer_size The size of the buffer (PDU_FIXED_FIELDS_LENGTH + payload length). Payload length depends on message type. 
 * @param[out]  pdu         Protocol Data Unit (PDU_S) structure.
 */
void deserialize_pdu(const uint8_t *buffer, const size_t buffer_size, PDU_S *pdu);

/**
 * @brief Create PDU for Connection Request.
 * 
 * @param[in]   self    State machine context structure.   
 * @param[in]   pdu     Pointer to the pdu that need to be build.
 */
void ConnReq(const SmType *self, PDU_S *pdu);

/**
 * @brief Create PDU for Connection Response.
 * 
 * @param[in]   self    State machine context structure. 
 * @param[in]   pdu     Pointer to the pdu that need to be build.
 */
void ConnResp(const SmType *self, PDU_S *pdu);

/**
 * @brief Create PDU for Retransmission Request.
 * 
 * @param[in]   self    State machine context structure.  
 * @param[in]   pdu     Pointer to the pdu that need to be build.
 */
void RetrReq(const SmType *self, PDU_S *pdu);

/**
 * @brief Create PDU for Retransmission Response.
 * 
 * @param[in]   self    State machine context structure.
 * @param[in]   pdu     Pointer to the pdu that need to be build.
 */
void RetrResp(const SmType *self, PDU_S *pdu);

/**
 * @brief Create PDU for Disconnection Request.
 * 
 * @param[in]   self            State machine context structure.  
 * @param[in]   pdu             Pointer to the pdu that need to be build. 
 * @param[in]   discReason      Disconnection reason.
 * @param[in]   detailedReason  If user request disconnection, then a detail about disconnection should be passed.
 */
void DiscReq(const SmType *self, PDU_S *pdu, DiscReasonType discReason, uint16_t detailedReason);

/**
 * @brief Create PDU for Heartbeat.
  * 
 * @param[in]   self    State machine context structure.
 * @param[in]   pdu     Pointer to the pdu that need to be build. 
 */
void HB(const SmType *self, PDU_S *pdu);

/**
 * @brief Create PDU for Data.
  * 
 * @param[in]   self        State machine context structure. 
 * @param[in]   pdu         Pointer to the pdu that need to be build. 
 * @param[in]   msgLen      The length of the data that will be transmitted.
 * @param[in]   pMsgData    Pointer to the data that will be transmitted.
 */
void Data(const SmType *self, PDU_S *pdu, const uint8_t msgLen, const uint8_t *pMsgData);

#endif // PDU_H
