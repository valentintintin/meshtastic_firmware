#if USE_TEXTCOMMANDMODULE

#include "configuration.h"
#include "MeshService.h"

#include <cstring>
#include <cassert>
#include <main.h>
#include <meshUtils.h>
#include <RTC.h>

#include "TextCommandModule.h"

Beacon TextCommandModule::beacon{};

TextCommandModule::TextCommandModule() : SinglePortModule("reply", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("TextCommandModule") {
    parser.registerCommand("!ping", "", doPing);
    parser.registerCommand("!voisins", "", doNeighbors);
    parser.registerCommand("!noeuds", "", doNodes);
    parser.registerCommand("!noeud", "s", doSearchNode);
    parser.registerCommand("!balise", "su", doBeacon);
    parser.registerCommand("!gpio", "uu", doGpioSet);
    parser.registerCommand("!gpioGet", "u", doGpioGet);
    parser.registerCommand("!gpioGetAdc", "u", doGpioGetAdc);
    parser.registerCommand("!set", "ss", doSetConfig);
    parser.registerCommand("!aide", "", doHelp);
    parser.registerCommand("!help", "", doHelp);
}

int32_t TextCommandModule::runOnce() {
    if (const auto newDelay = sendBeacon()) {
        return newDelay;
    }

    return THREAD_INTERVAL;
}

bool TextCommandModule::wantPacket(const meshtastic_MeshPacket *p) {
    return MeshService::isTextPayload(p) && p->decoded.payload.bytes[0] == '!' && (isToUs(p) || isBroadcast(p->to));
}

void TextCommandModule::alterReceived(meshtastic_MeshPacket &mp) {
    mp.decoded.want_response = true;
}

meshtastic_MeshPacket *TextCommandModule::allocReply()
{
    assert(currentRequest); // should always be !NULL

#ifdef DEBUG_PORT
    const auto req = *currentRequest;
    auto &p = req.decoded;
    // The incoming message is in p.payload
    LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", req.from, req.id, p.payload.size, reinterpret_cast<const char *>(p.payload.bytes));
#endif

    const auto reply = allocDataPacket();                 // Allocate a packet for sending
    setReplyTo(reply, *currentRequest);
    reply->channel = 0; // only by private message
    reply->want_ack = false;

    processCommand(reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes));
    reply->decoded.payload.size = strlen(tempBuffer);
    memcpy(reply->decoded.payload.bytes, tempBuffer, reply->decoded.payload.size);

#ifdef DEBUG_PORT
    LOG_INFO("Reply for command from=0x%0x, id=%d, msg=%.*s", req.from, req.id, reply->decoded.payload.size, reinterpret_cast<const char *>(reply->decoded.payload.bytes));
#endif

    return reply;
}

bool TextCommandModule::processCommand(const char *command) {
    if (!parser.processCommand(command, tempBuffer)) {
        LOG_WARN("Failed to parse command so help");

        if (IS_ONE_OF(config.device.role, meshtastic_Config_DeviceConfig_Role_ROUTER, meshtastic_Config_DeviceConfig_Role_ROUTER_LATE, meshtastic_Config_DeviceConfig_Role_REPEATER)) {
            doPing(nullptr, tempBuffer);
            strncat(tempBuffer, "\n\n!aide", MyCommandParser::MAX_RESPONSE_SIZE - strlen(tempBuffer));
        }

        return false;
    }

    return true;
}

uint64_t TextCommandModule::sendBeacon() {
    if (beacon.to == nullptr || beacon.nbTxLeft == 0) {
        return 0;
    }

    beacon.nbTxLeft--;

    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = beacon.to->num;
    p->channel = 0;
    p->priority = meshtastic_MeshPacket_Priority_RELIABLE;
    p->hop_limit = beacon.to->has_hops_away ? beacon.to->hops_away : config.lora.hop_limit;

    snprintf(tempBuffer, MyCommandParser::MAX_RESPONSE_SIZE, "!ping %llu\nSNR: %.2f", beacon.nbTxLeft, beacon.to->snr);
    p->decoded.payload.size = strlen(tempBuffer);
    memcpy(p->decoded.payload.bytes, tempBuffer, p->decoded.payload.size);

    service->sendToMesh(p, RX_SRC_LOCAL, true);

    return THREAD_INTERVAL * (beacon.to->hops_away + 1);
}

void TextCommandModule::doPing(MyCommandParser::Argument *args, char *response) {
    assert(currentRequest); // should always be !NULL

    strncpy(response, "Pong !\n\n", MyCommandParser::MAX_RESPONSE_SIZE);

    if (!isToUs(currentRequest)) {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "%.30s[..]\n\n", reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes));
    }

    const auto nbHops = currentRequest->hop_start - currentRequest->hop_limit;

    if (nbHops == 0) {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Direct. RSSI: %d SNR: %.2f", currentRequest->rx_rssi, currentRequest->rx_snr);
    } else {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Sauts: %d/%d", nbHops, currentRequest->hop_limit);
    }
}

void TextCommandModule::doNodes(MyCommandParser::Argument *args, char *response) {
    listNodes(response, nodeDB->numMeshNodes >= 20 ? 24 : INT_MAX, false);
}

void TextCommandModule::doNeighbors(MyCommandParser::Argument *args, char *response) {
    listNodes(response, 24, true);
}

void TextCommandModule::doSearchNode(MyCommandParser::Argument *args, char *response) {
    const auto nodeIdOrName = args[0].asString;
    const auto node = findNode(nodeIdOrName);

    if (!node) {
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "%s pas entendu", nodeIdOrName);
        return;
    }

    LOG_DEBUG("Found node with id 0x%x", node->num);

    const time_t epochTimeT = node->last_heard;
    const tm ts = *localtime(&epochTimeT);

    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "!%x", node->num);

    if (node->has_user) {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), " -> %s\n%s", node->user.short_name, node->user.long_name);
    }

    snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nEntendu: %d-%d-%dT%d:%d:%dZ (%lu)\n",
            ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec, node->last_heard);

    if (node->has_hops_away) {
        if (node->hops_away == 0) {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Direct. SNR: %.2f", node->snr);
        } else {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Sauts: %d", node->hops_away);
        }
    }
}

void TextCommandModule::doHelp(MyCommandParser::Argument *args, char *response) {
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Aide :\n!ping\n!voisins\n!noeuds\n!noeud NOM_COURT\n!balise NOM_COURT NB_FOIS");
}

void TextCommandModule::doGpioSet(MyCommandParser::Argument *args, char *response) {
    const auto pin = args[0].asUInt64;
    const auto status = args[1].asUInt64 == 1 ? HIGH : LOW;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, status);

    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pin %llu -> %s", pin, status == HIGH ? "ON" : "OFF");
}

void TextCommandModule::doGpioGet(MyCommandParser::Argument *args, char *response) {
    const auto pin = args[0].asUInt64;
    pinMode(pin, INPUT);
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pin %llu -> %d", pin, digitalRead(pin));
}

void TextCommandModule::doGpioGetAdc(MyCommandParser::Argument *args, char *response) {
    const auto pin = args[0].asUInt64;
    pinMode(pin, INPUT);
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pin %llu -> %d", pin, analogRead(pin));
}

void TextCommandModule::doSetConfig(MyCommandParser::Argument *args, char *response) {
    char *key = args[0].asString;
    char *value = args[1].asString;

    if (strlen(value) == 0) {
        LOG_WARN("Set %s to nothing impossible", key);
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "KO set %s to nothing", key);
        return;
    }

    auto changes = SEGMENT_CONFIG;
    bool ok = true;
    bool shouldReboot = false;
    LOG_INFO("Set %s to %s", key, value);

    if (strcmp(key, "tx") == 0) {
        config.lora.tx_enabled = value[0] == '1';
    } else {
        ok = false;
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "KO %s not found", key);
    }

    if (ok) {
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Set %s to %s OK. Reboot: %d", key, value, shouldReboot);
        service->reloadConfig(changes); // Calls saveToDisk among other things

        if (shouldReboot) {
            LOG_INFO("Reboot in %d seconds !", DEFAULT_REBOOT_SECONDS);
            rebootAtMsec = millis() + DEFAULT_REBOOT_SECONDS * 1000;
        }
    }
}

void TextCommandModule::doBeacon(MyCommandParser::Argument *args, char *response) {
    const auto nodeIdOrName = args[0].asString;
    const auto node = findNode(nodeIdOrName);

    if (!node) {
        beacon.to = nullptr;
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "%s pas trouvé", nodeIdOrName);
        return;
    }

    LOG_DEBUG("Found node with id 0x%x", node->num);

    beacon.to = node;
    beacon.nbTxLeft = args[1].asUInt64;

    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Balise activé en direction de !%x toutes les %d secondes (%d sauts) et %llu fois",
        node->num, THREAD_INTERVAL * (node->hops_away + 1) / 1000,
        node->has_hops_away ? node->hops_away : config.lora.hop_limit,
        beacon.nbTxLeft);

    LOG_DEBUG("Beacon OK %s", response);
}

void TextCommandModule::listNodes(char *buffer, int hoursLastHeard, bool onlyNeighbors) {
    const auto now = getTime();
    const auto maxTime = 3600 * hoursLastHeard;

    for (const auto& node : *nodeDB->meshNodes) {
        if (node.num == nodeDB->getNodeNum()) continue;
        if (strlen(buffer) >= sizeof(buffer) - (onlyNeighbors ? 5 : 10)) return;
        if (now - node.last_heard > maxTime) {
            LOG_DEBUG("Node 0x%x not heard for %d hours", node.num, hoursLastHeard);
            continue;
        }
        if (onlyNeighbors && (!node.has_hops_away || node.hops_away != 0)) {
            LOG_DEBUG("Node 0x%x not a direct neighbor", node.num);
            continue;
        }

        if (strlen(buffer) > 0) {
            strncat(buffer, "\n", MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer));
        }

        if (node.has_user) {
            strncat(buffer, node.user.short_name, MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer));
        } else {
            snprintf(buffer + strlen(buffer), MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), "%x", node.num & 0xFFFF);
        }
        if (!onlyNeighbors && node.has_hops_away) {
            snprintf(buffer + strlen(buffer), MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), " (%d)", node.hops_away);
        }
    }

    if (strlen(buffer) == 0) {
        snprintf(buffer, MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), "Personne depuis %d heures", hoursLastHeard);
    }
}

const _meshtastic_NodeInfoLite *TextCommandModule::findNode(char *nodeIdOrName) {
    const auto nodeIdNum = static_cast<uint16_t>(strtol(nodeIdOrName, nullptr, 16));

    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        const auto& node = nodeDB->meshNodes->at(i);

        if ((node.has_user && strcasecmp(node.user.short_name, nodeIdOrName) == 0)
            || (nodeIdNum > 0 && nodeIdNum == static_cast<uint16_t>(node.num & 0xFFFF))
        ) {
            return &node;
        }
    }

    return nullptr;
}

#endif