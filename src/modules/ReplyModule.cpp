#include "ReplyModule.h"
#include "configuration.h"
#include "MeshService.h"

#include <string.h>
#include <assert.h>

bool ReplyModule::wantPacket(const meshtastic_MeshPacket *p) {
    return MeshService::isTextPayload(p);
}

void ReplyModule::alterReceived(meshtastic_MeshPacket &mp) {
    if (isToUs(&mp) || mp.decoded.payload.bytes[0] == '!') {
        mp.decoded.want_response = true;
    }
}

meshtastic_MeshPacket *ReplyModule::allocReply()
{
    assert(currentRequest); // should always be !NULL
#ifdef DEBUG_PORT
    auto req = *currentRequest;
    auto &p = req.decoded;
    // The incoming message is in p.payload
    LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", req.from, req.id, p.payload.size, p.payload.bytes);
#endif

    sprintf(tempBuffer, ">%.30s[...]\nSauts: %d | RSSI: %d | SNR: %.2f", currentRequest->decoded.payload.bytes,
            currentRequest->hop_start - currentRequest->hop_limit, currentRequest->rx_rssi, currentRequest->rx_snr);

    auto reply = allocDataPacket();                 // Allocate a packet for sending
    reply->to = currentRequest->from;
    reply->decoded.payload.size = strlen(tempBuffer); // You must specify how many bytes are in the reply
    memcpy(reply->decoded.payload.bytes, tempBuffer, reply->decoded.payload.size);

    return reply;
}
