#include "protocol.h"
#include "stdlib.h"
#include "string.h"


PDU *mkPDU(uint uiMsgLen)
{
    if (uiMsgLen > 100 * 1024 * 1024) {
        return NULL;
    }
    uint uiTotalLen=uiMsgLen+sizeof(PDU);
    if (uiTotalLen < uiMsgLen) {
        return NULL;
    }
    PDU* pdu=(PDU*)malloc(uiTotalLen);
    if(pdu==NULL){
        return NULL;
    }
    memset(pdu,0,uiTotalLen);
    pdu->uiTotalLen=uiTotalLen;
    pdu->uiMsgLen=uiMsgLen;
    return pdu;
}
