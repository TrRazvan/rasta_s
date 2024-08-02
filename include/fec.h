#ifndef FEC_H
#define FEC_H

#include "types.h"

StdRet_t Fec_ReceiveBtp(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

#endif /* FEC_H */
