#include "ReplyModule.h"
#include "configuration.h"
#include "MeshService.h"

#include <cstring>
#include <cassert>
#include <RTC.h>

#include "NeighborInfoModule.h"

ReplyModule::ReplyModule() : SinglePortModule("reply", meshtastic_PortNum_TEXT_MESSAGE_APP) {
    parser.registerCommand("!ping", "", doPing);
    parser.registerCommand("!voisins", "", doNeighbors);
    parser.registerCommand("!voisin", "s", doSearchNeighbor);
    parser.registerCommand("!help", "", doHelp);
}

bool ReplyModule::wantPacket(const meshtastic_MeshPacket *p) {
    return MeshService::isTextPayload(p) && p->decoded.payload.bytes[0] == '!' && (isToUs(p) || isBroadcast(p->to));
}

void ReplyModule::alterReceived(meshtastic_MeshPacket &mp) {
    mp.decoded.want_response = true;
}

meshtastic_MeshPacket *ReplyModule::allocReply()
{
    assert(currentRequest); // should always be !NULL

#ifdef DEBUG_PORT
    const auto req = *currentRequest;
    auto &p = req.decoded;
    // The incoming message is in p.payload
    LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", req.from, req.id, p.payload.size, reinterpret_cast<const char *>(p.payload.bytes));
#endif

    const auto reply = allocDataPacket();                 // Allocate a packet for sending

    reply->to = currentRequest->from;
    reply->channel = 0;

    processCommand(reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes));

    reply->decoded.payload.size = strlen(tempBuffer);
    memcpy(reply->decoded.payload.bytes, tempBuffer, reply->decoded.payload.size);

    return reply;
}

bool ReplyModule::processCommand(const char *command) {
    if (!parser.processCommand(command, tempBuffer)) {
        LOG_WARN("Failed to parse command so help");

        tempBuffer[0] = '\0';
        doPing(nullptr, tempBuffer);

        return false;
    }

    return true;
}

void ReplyModule::doPing(MyCommandParser::Argument *args, char *response) {
    assert(currentRequest); // should always be !NULL

    if (!isToUs(currentRequest)) {
        sprintf(response, "%.30s\n", reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes));
    }

    strcat(response, "Pong !\n");

    const auto nbHops = currentRequest->hop_start - currentRequest->hop_limit;

    if (nbHops == 0) {
        sprintf(response + strlen(response), "Direct. RSSI: %d SNR: %.2f", currentRequest->rx_rssi, currentRequest->rx_snr);
    } else {
        sprintf(response + strlen(response), "Sauts: %d/%d", nbHops, currentRequest->hop_limit);
    }

    strcat(response, "\n\n");

    doHelp(args, response + strlen(response));
}

void ReplyModule::doNeighbors(MyCommandParser::Argument *args, char *response) {
    const auto now = getTime();
    constexpr int hours = 2;
    constexpr int maxTime = 3600 * hours;

    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        const auto neighbor = nodeDB->meshNodes->at(i);

        if (neighbor.has_hops_away && neighbor.hops_away == 0 && neighbor.has_user) {
            if (strlen(response) > 0) {
                strcat(response, " ");
            }

            if (now - neighbor.last_heard <= maxTime) {
                strcat(response, neighbor.user.short_name);
            }
        }
    }

    if (strlen(response) == 0) {
        sprintf(response, "Personne depuis %d heures", hours);
    }
}

void ReplyModule::doSearchNeighbor(MyCommandParser::Argument *args, char *response) {
    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        const auto neighbor = nodeDB->meshNodes->at(i);

        if (neighbor.has_user && strcasecmp(neighbor.user.short_name, args[0].asString) == 0) {
            const time_t epochTimeT = neighbor.last_heard;
            const tm ts = *localtime(&epochTimeT);
            sprintf(response, "0x%x -> %s\n%s\nSNR: %.2f\nEntendu: %d-%d-%dT%d:%d:%dZ", neighbor.num, neighbor.user.short_name, neighbor.user.long_name, neighbor.snr,
                ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
            return;
        }
    }

    if (strlen(response) == 0) {
        sprintf(response, "%s pas entendu", args[0].asString);
    }
}

void ReplyModule::doHelp(MyCommandParser::Argument *args, char *response) {
    sprintf(response, "!ping !voisins !voisin NOM_COURT");
}
