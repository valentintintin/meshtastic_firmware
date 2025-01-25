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
        sprintf(tempBuffer, "%.160s\n->%s(%d/%d)|RSSI:%d|SNR:%.2f", mp.decoded.payload.bytes + 1,
            owner.short_name, mp.hop_start - mp.hop_limit, mp.hop_limit, mp.rx_rssi, mp.rx_snr);
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

    sprintf(tempBuffer, ">%.30s[...]\nSauts: %d | RSSI: %d | SNR: %.2f", currentRequest->decoded.payload.bytes,
        currentRequest->hop_start - currentRequest->hop_limit, currentRequest->rx_rssi, currentRequest->rx_snr);
    reply->to = currentRequest->from;
    reply->channel = 0;

    reply->decoded.payload.size = strlen(tempBuffer); // You must specify how many bytes are in the reply
    memcpy(reply->decoded.payload.bytes, tempBuffer, reply->decoded.payload.size);

    return reply;
}
