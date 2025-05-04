#if !MESHTASTIC_EXCLUDE_TEXTCOMMAND

#include "NeighborInfoModule.h"
#include "NodeInfoModule.h"
#include "PositionModule.h"
#include "SerialModule.h"
#include "Telemetry/DeviceTelemetry.h"
#include "Telemetry/EnvironmentTelemetry.h"
#include "Telemetry/PowerTelemetry.h"

#include "configuration.h"
#include "MeshService.h"
#include "Channels.h"

#include <cstring>
#include <cassert>
#include <main.h>
#include <meshUtils.h>
#include <RTC.h>

#include "TextCommandModule.h"

// #ifdef HAS_SLAVE_SENSOR
    // #include "Telemetry/Sensor/MySlaveSensors/MySlaveSensor.h"
    // MySlaveSensor mySlaveSensor("SlaveSensor");
// #endif

TextCommandModule* textCommandModule{};
Beacon TextCommandModule::beacon{};
bool TextCommandModule::shouldReloadConfig = false;
meshtastic_Config_LoRaConfig_ModemPreset TextCommandModule::oldLoRaModemPreset;
char TextCommandModule::oldPrimaryChannelName[12];
meshtastic_NodeInfoLite *TextCommandModule::sortedNodeHeards[MAX_NUM_NODES];
bool TextCommandModule::updateProtoSerial = false;

TextCommandModule::TextCommandModule() : SinglePortModule("textCommand", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("TextCommandModule") {
    parser.registerCommand("!ping", "", doPing);
    parser.registerCommand("!voisins", "", doNeighbors);
    parser.registerCommand("!noeuds", "", doNodes);
    parser.registerCommand("!noeud", "s", doSearchNode);
    parser.registerCommand("!balise", "su", doBeacon);
    parser.registerCommand("!gpio", "uu", doGpioSet);
    parser.registerCommand("!gpioGet", "u", doGpioGet);
    parser.registerCommand("!gpioGetAdc", "u", doGpioGetAdc);
    parser.registerCommand("!set", "ss", doSetConfig);
    parser.registerCommand("!get", "s", doGet);
    parser.registerCommand("!ask", "s", doAsk);
    parser.registerCommand("!msg", "sss", doSendMessage);
#ifdef HAS_SLAVE_SENSOR
    parser.registerCommand("!cmdSlave", "s", doCommandMySlaveSensor);
    parser.registerCommand("!cmdRepSlave", "s", doGetResponseCommandMySlaveSensor);
#endif
}

int32_t TextCommandModule::runOnce() {
    isRouter = IF_ROUTER(true, false);

    if (updateProtoSerial) {
        serialModule->setAsConnected(true);
    }

    const auto txQueueStatus = router->getQueueStatus();
    if (shouldReloadConfig && txQueueStatus.free == txQueueStatus.maxlen) {
        LOG_INFO("We need to reload config");

        if (config.lora.modem_preset != oldLoRaModemPreset) {
            LOG_WARN("Set LoRa preset to %u", oldLoRaModemPreset);
            config.lora.modem_preset = oldLoRaModemPreset;
            service->configChanged.notifyObservers(nullptr);
        }

        auto channel = channels.getByIndex(channels.getPrimaryIndex());
        if (strcasecmp(channel.settings.name, oldPrimaryChannelName) != 0) {
            LOG_WARN("Set primary channel name to %s", oldPrimaryChannelName);
            strncpy(channel.settings.name, oldPrimaryChannelName, 12);
            channels.setChannel(channel);
            channels.onConfigChanged();
        }

        shouldReloadConfig = false;
    } else if (const auto newDelay = sendBeacon()) {
        return newDelay;
    }

    return THREAD_INTERVAL;
}

bool TextCommandModule::wantPacket(const meshtastic_MeshPacket *p) {
    return MeshService::isTextPayload(p)
           && (isToUs(p)
               || (
                   p->decoded.payload.bytes[0] == '!'
                   && isBroadcast(p->to)
                   && isRouter
               )
           );
}

ProcessMessage TextCommandModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    memset(response, '\0', MyCommandParser::MAX_RESPONSE_SIZE);

    LOG_INFO("Received message for reply from=0x%0x, id=%d, msg=%.*s", mp.from, mp.id, mp.decoded.payload.size, reinterpret_cast<const char *>(mp.decoded.payload.bytes));

    if (!processCommand(reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes))) {
        return ProcessMessage::CONTINUE;
    }

    const auto reply = allocDataPacket();                 // Allocate a packet for sending
    setReplyTo(reply, mp);

    reply->decoded.payload.size = strlen(response);
    memcpy(reply->decoded.payload.bytes, response, reply->decoded.payload.size);

    LOG_INFO("Reply for command from=0x%0x, id=%d, msg=%.*s", reply->from, reply->id, reply->decoded.payload.size, reinterpret_cast<const char *>(reply->decoded.payload.bytes));

    service->sendToMesh(reply);

    return ProcessMessage::CONTINUE;
}

bool TextCommandModule::processCommand(const char *command) {
    if (!parser.processCommand(command, response)) {
        LOG_WARN("Failed to parse command %s", command);

        if (!isRouter) {
            return false;
        }

        doPing(nullptr, response);
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
    p->hop_limit = beacon.to->has_hops_away ? beacon.to->hops_away : config.lora.hop_limit;

    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "!ping %llu\nSNR: %.2f", beacon.nbTxLeft, beacon.to->snr);

    p->decoded.payload.size = strlen(response);
    memcpy(p->decoded.payload.bytes, response, p->decoded.payload.size);

    service->sendToMesh(p);

    return THREAD_INTERVAL * (beacon.to->hops_away + 1);
}

bool TextCommandModule::sendMessage(char modemPresetName[2], char channelName[12], char message[200]) {
    if (!router) {
        return false;
    }

    if (beacon.nbTxLeft > 0) {
        return false;
    }

    auto modemPreset = config.lora.modem_preset;
    auto channel = channels.getByIndex(channels.getPrimaryIndex());

    oldLoRaModemPreset = config.lora.modem_preset;
    strncpy(oldPrimaryChannelName, channel.settings.name, 12);

    LOG_INFO("Want to send message %s with preset %s on channel %s", message, modemPresetName, channelName);

    if (strcasecmp(channel.settings.name, channelName) != 0) {
        LOG_WARN("Set primary channel name to %s", channelName);
        strncpy(channel.settings.name, channelName, 12);
        channels.setChannel(channel);
        channels.onConfigChanged();
        shouldReloadConfig = true;
    }

    if (strcasecmp(modemPresetName, "LF") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_LONG_FAST;
    } else if (strcasecmp(modemPresetName, "LM") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_LONG_MODERATE;
    } else if (strcasecmp(modemPresetName, "LS") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_LONG_SLOW;
    } else if (strcasecmp(modemPresetName, "MF") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_FAST;
    } else if (strcasecmp(modemPresetName, "MS") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_SLOW;
    } else if (strcasecmp(modemPresetName, "SF") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_SHORT_FAST;
    } else if (strcasecmp(modemPresetName, "SS") == 0) {
        modemPreset = meshtastic_Config_LoRaConfig_ModemPreset_SHORT_SLOW;
    }

    if (config.lora.modem_preset != modemPreset) {
        LOG_WARN("Change LoRa preset to %u", modemPreset);
        config.lora.modem_preset = modemPreset;
        service->configChanged.notifyObservers(nullptr);
        shouldReloadConfig = true;
    }

    const auto p = router->allocForSending();
    p->channel = channel.index;
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    p->decoded.payload.size = strlen(message);
    memcpy(p->decoded.payload.bytes, message, p->decoded.payload.size);

    router->send(p);

    if (shouldReloadConfig) {
        setInterval(THREAD_INTERVAL);
    }

    return true;
}

void TextCommandModule::doPing(MyCommandParser::Argument *args, char *response) {
    if (currentRequest == nullptr) {
        strncpy(response, "Pong KO", MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    strncpy(response, "Pong !\n\n", MyCommandParser::MAX_RESPONSE_SIZE);

    if (!isToUs(currentRequest)) {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "%.30s[..]\n\n", reinterpret_cast<const char *>(currentRequest->decoded.payload.bytes));
    }

    const auto nbHops = currentRequest->hop_start - currentRequest->hop_limit;

    if (nbHops == 0) {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Direct. SNR: %.2f RSSI: %d", currentRequest->rx_snr, currentRequest->rx_rssi);
    } else {
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Sauts: %d/%d", nbHops, currentRequest->hop_limit);

        if (currentRequest->relay_node != 0) {
            const auto relayNode = findNeighborNodeFromLastByte(currentRequest->relay_node);

            if (relayNode != nullptr) {
                snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nVia: !%x", relayNode->num);

                if (relayNode->has_user) {
                    snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), " -> %s\n%s", relayNode->user.short_name, relayNode->user.long_name);
                }
            } else {
                snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nVia: 0x%x", currentRequest->relay_node);
            }
        }
    }
}

void TextCommandModule::doNodes(MyCommandParser::Argument *args, char *response) {
    listNodes(response, false);
}

void TextCommandModule::doNeighbors(MyCommandParser::Argument *args, char *response) {
    listNodes(response, true);
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

    snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nEntendu: %d-%d-%dT%d:%d:%dZ\n",
            ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);

    if (node->has_hops_away) {
        if (node->hops_away == 0) {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Direct. SNR: %.2f", node->snr);
        } else {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "Sauts: %d", node->hops_away);
        }
    }

    if (node->next_hop > 0) {
        const auto nextHop = findNeighborNodeFromLastByte(node->next_hop);

        if (nextHop != nullptr) {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nVia: !%x", nextHop->num);

            if (nextHop->has_user) {
                snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), " -> %s\n%s", nextHop->user.short_name, nextHop->user.long_name);
            }
        } else {
            snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "\nVia: 0x%x", node->next_hop);
        }
    }
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

    if (strcasecmp(key, "tx") == 0) {
        config.lora.tx_enabled = value[0] == '1';
    } else if (strcasecmp(key, "preset") == 0) {
        config.lora.modem_preset = static_cast<meshtastic_Config_LoRaConfig_ModemPreset>(strtoul(value, nullptr, 0));
    } else if (strcasecmp(key, "role") == 0) {
        config.device.role = static_cast<meshtastic_Config_DeviceConfig_Role>(strtoul(value, nullptr, 0));
    } else if (strcasecmp(key, "primaryChannel") == 0) {
        auto channel = channels.getByIndex(channels.getPrimaryIndex());
        if (strcasecmp(channel.settings.name, value) != 0) {
            LOG_WARN("Set primary channel name to %s", value);
            strncpy(channel.settings.name, value, 12);
            channels.setChannel(channel);
            channels.onConfigChanged();
            changes = SEGMENT_CHANNELS;
        }
    } else if (strcasecmp(key, "power") == 0) {
        config.lora.tx_power = static_cast<int8_t>(strtol(value, nullptr, 0));
    } else if (strcasecmp(key, "neighborInfo") == 0) {
        moduleConfig.neighbor_info.enabled = value[0] == '1';
        moduleConfig.neighbor_info.transmit_over_lora = true;
        changes = SEGMENT_MODULECONFIG;
        shouldReboot = true;
    } else if (strcasecmp(key, "admin") == 0) {
        const auto node = findNode(value);

        if (node != nullptr) {
            uint8_t adminKeyIndex = config.security.admin_key_count;
            if (config.security.admin_key_count >= 3) {
                adminKeyIndex = 2; // On remplace la dernière clé
            }

            memcpy(config.security.admin_key[adminKeyIndex].bytes, node->user.public_key.bytes, node->user.public_key.size);
            config.security.admin_key[adminKeyIndex].size = node->user.public_key.size;
            config.security.admin_key_count = adminKeyIndex + 1;
            shouldReboot = true;
        } else {
            ok = false;
            snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "KO %s not found", value);
        }
    }  else if (strcasecmp(key, "remove") == 0) {
        const auto node = findNode(value);

        if (node != nullptr) {
            nodeDB->removeNodeByNum(node->num);
        } else {
            ok = false;
            snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "KO %s not found", value);
        }
    } else if (strcasecmp(key, "reset") == 0) {
        nodeDB->resetRadioConfig(strcmp(value, "all") == 0);
        shouldReboot = true;
    } else if (strcasecmp(key, "name") == 0) {
        if (strcmp(owner.long_name, value) == 0) {
            strncpy(owner.long_name, value, sizeof(owner.long_name));
            service->reloadOwner();
            service->reloadConfig(SEGMENT_DEVICESTATE | SEGMENT_NODEDATABASE);
            shouldReboot = true;
        }
    } else if (strcasecmp(key, "shortName") == 0) {
        if (strcmp(owner.short_name, value) == 0) {
            strncpy(owner.short_name, value, sizeof(owner.short_name));
            service->reloadOwner();
            service->reloadConfig(SEGMENT_DEVICESTATE | SEGMENT_NODEDATABASE);
            shouldReboot = true;
        }
    } else if (strcasecmp(key, "serialProto") == 0) {
        updateProtoSerial = value[0] == '1';
        if (updateProtoSerial) {
            LOG_DEBUG("Set phone is connected (virtually)");
        }
        serialModule->setAsConnected(updateProtoSerial);
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

    if (!node || !node->has_user || node->user.public_key.size == 0) {
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

void TextCommandModule::doAsk(MyCommandParser::Argument *args, char *response) {
    const auto what = args[0].asString;

#if !MESHTASTIC_EXCLUDE_NODEINFO
    if (strcasecmp(what, "nodeinfo") == 0) {
        nodeInfoModule->sendOurNodeInfo();
    }
#else
    if (false) { // only for others elseif }
#endif
#if !MESHTASTIC_EXCLUDE_GPS
    else if (strcasecmp(what, "position") == 0) {
        positionModule->sendOurPosition();
    }
#endif
#if !MESHTASTIC_EXCLUDE_NEIGHBORINFO
    else if (strcasecmp(what, "voisins") == 0 && moduleConfig.neighbor_info.enabled) {
        neighborInfoModule->sendNeighborInfo();
    }
#endif
#if HAS_TELEMETRY
    else if (strcasecmp(what, "telem") == 0) {
        deviceTelemetryModule->sendTelemetry();
    }
#if !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
    else if (strcasecmp(what, "meteo") == 0) {
        environmentTelemetryModule->sendTelemetry();
    }
#if !MESHTASTIC_EXCLUDE_POWER_TELEMETRY
    else if (strcasecmp(what, "power") == 0) {
        powerTelemetryModule->sendTelemetry();
    }
#endif
#endif
#endif
#if defined(ARCH_NRF52) || defined(ARCH_RP2040)
    else if (strcasecmp(what, "dfu") == 0) {
        LOG_INFO("Client requesting to enter DFU mode");
        enterDfuMode();
    }
#endif
    else if (strcasecmp(what, "reboot") == 0) {
        screen->startAlert("Rebooting...");
        rebootAtMsec = millis() + 5000;
    } else {
        strncpy(response, "KO pas compris", MyCommandParser::MAX_RESPONSE_SIZE);
        return;
    }

    strncpy(response, "OK", MyCommandParser::MAX_RESPONSE_SIZE);
}

void TextCommandModule::doGet(MyCommandParser::Argument *args, char *response) {
    const auto what = args[0].asString;

    if (strcasecmp(what, "time") == 0) {
        const time_t epochTimeT = getTime();
        const tm ts = *localtime(&epochTimeT);
        snprintf(response + strlen(response), MyCommandParser::MAX_RESPONSE_SIZE - strlen(response), "%d-%d-%dT%d:%d:%dZ",
                ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
    } else if (strcasecmp(what, "primaryChannel") == 0) {
        auto channel = channels.getByIndex(channels.getPrimaryIndex());
        strncpy(response, channel.settings.name, MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "KO pas compris", MyCommandParser::MAX_RESPONSE_SIZE);
    }
}

void TextCommandModule::doSendMessage(MyCommandParser::Argument *args, char *response) {
    if (textCommandModule != nullptr && textCommandModule->sendMessage(args[0].asString, args[1].asString, args[2].asString)) {
        strncpy(response, "OK", MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "KO", MyCommandParser::MAX_RESPONSE_SIZE);
    }
}

#ifdef HAS_SLAVE_SENSOR
void TextCommandModule::doCommandMySlaveSensor(MyCommandParser::Argument *args, char *response) {
    // TODO Not implemented
    strncpy(response, "OK", MyCommandParser::MAX_RESPONSE_SIZE);
}

void TextCommandModule::doGetResponseCommandMySlaveSensor(MyCommandParser::Argument *args, char *response) {
    // TODO Not implemented
    strncpy(response, "OK", MyCommandParser::MAX_RESPONSE_SIZE);
}
#endif

void TextCommandModule::listNodes(char *buffer, bool onlyNeighbors) {
    memset(sortedNodeHeards, 0, sizeof(meshtastic_NodeInfoLite *) * MAX_NUM_NODES);
    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        sortedNodeHeards[i] = &nodeDB->meshNodes->at(i);
    }

    qsort(sortedNodeHeards, nodeDB->numMeshNodes, sizeof(meshtastic_NodeInfoLite *), compareNodesHeardTimeDescending);

    for (const auto node : sortedNodeHeards) {
        LOG_WARN("Node 0x%x last_heard %lu", node->num, node->last_heard);
        if (node->num == nodeDB->getNodeNum()) {
            continue;
        }

        if (strlen(buffer) + (onlyNeighbors ? 5 : 8) >= MyCommandParser::MAX_RESPONSE_SIZE) {
            return;
        }

        if (onlyNeighbors && (!node->has_hops_away || node->hops_away != 0)) {
            LOG_DEBUG("Node 0x%x not a direct neighbor", node->num);
            continue;
        }

        if (strlen(buffer) > 0) {
            strncat(buffer, "\n", MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer));
        }

        if (node->has_user) {
            strncat(buffer, node->user.short_name, MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer));
        } else {
            snprintf(buffer + strlen(buffer), MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), "%x", node->num & 0xFFFF);
        }
        if (!onlyNeighbors && node->has_hops_away) {
            snprintf(buffer + strlen(buffer), MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), ":%d", node->hops_away);
        }
    }

    if (strlen(buffer) == 0) {
        snprintf(buffer, MyCommandParser::MAX_RESPONSE_SIZE - strlen(buffer), "Personne entendu");
    }
}

const _meshtastic_NodeInfoLite *TextCommandModule::findNode(char *nodeIdOrName) {
    const auto nodeIdNum = static_cast<uint16_t>(strtol(nodeIdOrName, nullptr, 16));

    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        const auto& node = nodeDB->meshNodes->at(i);

        if ((node.has_user && strcasecmp(node.user.short_name, nodeIdOrName) == 0)
            || (nodeIdNum > 0 && (nodeIdNum == node.num || nodeIdNum == static_cast<uint16_t>(node.num & 0xFFFF)))
        ) {
            return &node;
        }
    }

    return nullptr;
}

const _meshtastic_NodeInfoLite *TextCommandModule::findNeighborNodeFromLastByte(uint8_t lastByte) {
    for (int i = 0; i < nodeDB->numMeshNodes; i++) {
        const auto& node = nodeDB->meshNodes->at(i);

        if (node.has_hops_away && node.hops_away == 0
            && lastByte == nodeDB->getLastByteOfNodeNum(node.num)
        ) {
            return &node;
        }
    }

    return nullptr;
}

int TextCommandModule::compareNodesHeardTimeDescending(const void *a, const void *b) {
    const auto first = *(const meshtastic_NodeInfoLite **)a;
    const auto second = *(const meshtastic_NodeInfoLite **)b;

    return (second->last_heard > first->last_heard) - (second->last_heard < first->last_heard);
}

#endif