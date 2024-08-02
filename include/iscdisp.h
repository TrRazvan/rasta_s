#ifndef ISCDISP_H
#define ISCDISP_H

#include "types.h"

StdRet_t IscDispApp_SendSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);

#endif /* ISCDISP_H */
