#include "ReplyModule.h"
#include "configuration.h"
#include "MeshService.h"

#include <string.h>
#include <assert.h>

bool ReplyModule::wantPacket(const meshtastic_MeshPacket *p) {
    return MeshService::isTextPayload(p) && (
            (moduleConfig.neighbor_info.enabled && moduleConfig.neighbor_info.transmit_over_lora)
            || config.device.role == meshtastic_Config_DeviceConfig_Role_ROUTER
            || config.device.role == meshtastic_Config_DeviceConfig_Role_ROUTER_LATE
            || config.device.role == meshtastic_Config_DeviceConfig_Role_REPEATER
    );
}

void ReplyModule::alterReceived(meshtastic_MeshPacket &mp) {
    if (isToUs(&mp)) {
        mp.decoded.want_response = true;
    } else if (isBroadcast(mp.to) && mp.decoded.payload.bytes[0] == '!') {
        createNewMessage(&mp);
    }
}

meshtastic_MeshPacket *ReplyModule::allocReply()
{
    assert(currentRequest); // should always be !NULL

    if (!isToUs(currentRequest)) {
        return nullptr;
    }

#ifdef DEBUG_PORT
    auto req = *currentRequest;
    auto &p = req.decoded;
    // The incoming message is in p.payload
    LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", req.from, req.id, p.payload.size, p.payload.bytes);
#endif

    const auto reply = allocDataPacket();                 // Allocate a packet for sending

    createNewMessage(currentRequest);

    reply->to = currentRequest->from;
    reply->channel = 0;

    reply->decoded.payload.size = strlen(tempBuffer); // You must specify how many bytes are in the reply
    memcpy(reply->decoded.payload.bytes, tempBuffer, reply->decoded.payload.size);

    return reply;
}

void ReplyModule::createNewMessage(const meshtastic_MeshPacket *p) {
#ifdef DEBUG_PORT
     LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", p->from, p->id, p->decoded.payload.size, p->decoded.payload.bytes);
#endif

    if (p->hop_start - p->hop_limit == 0) {
        sprintf(tempBuffer, "%.160s\nVia %s|Direct|RSSI:%d|SNR:%.2f", p->decoded.payload.bytes,
                owner.short_name, p->rx_rssi, p->rx_snr);
    } else {
        sprintf(tempBuffer, "%.160s\nVia %s|%d/%d", p->decoded.payload.bytes,
                owner.short_name, p->hop_start - p->hop_limit, p->hop_limit);
    }
}