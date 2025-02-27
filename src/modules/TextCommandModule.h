#ifndef MESHTASTIC_TEXTCOMMANDMODULE_H
#define MESHTASTIC_TEXTCOMMANDMODULE_H

#if USE_TEXTCOMMANDMODULE

#include "Observer.h"
#include "SinglePortModule.h"

#include <CommandParser.h>

#define THREAD_INTERVAL (15 * 1000) // 15 seconds

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<32,       3,              16,                     200,                0,              200> MyCommandParser;

typedef struct {
    const _meshtastic_NodeInfoLite *to;
    uint64_t nbTxLeft;
} Beacon;

class TextCommandModule : public SinglePortModule, public Observable<const meshtastic_MeshPacket *>, private concurrency::OSThread {
public:
    TextCommandModule();
    int32_t runOnce() override;

protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

private:
    char tempBuffer[MyCommandParser::MAX_RESPONSE_SIZE] = {};
    MyCommandParser parser;
    bool isRouter;

    bool processCommand(const char *command);
    uint64_t sendBeacon();
    bool sendMessage(char modemPresetName[2], char channelName[12], char message[MyCommandParser::MAX_RESPONSE_SIZE]);

    static TextCommandModule *instance;
    static Beacon beacon;
    static bool shouldReloadConfig;
    static meshtastic_Config_LoRaConfig_ModemPreset oldLoRaModemPreset;
    static char oldPrimaryChannelName[12];

    static void doPing(MyCommandParser::Argument *args, char *response);
    static void doNeighbors(MyCommandParser::Argument *args, char *response);
    static void doNodes(MyCommandParser::Argument *args, char *response);
    static void doSearchNode(MyCommandParser::Argument *args, char *response);
    static void doHelp(MyCommandParser::Argument *args, char *response);
    static void doGpioSet(MyCommandParser::Argument *args, char *response);
    static void doGpioGet(MyCommandParser::Argument *args, char *response);
    static void doGpioGetAdc(MyCommandParser::Argument *args, char *response);
    static void doSetConfig(MyCommandParser::Argument *args, char *response);
    static void doBeacon(MyCommandParser::Argument *args, char *response);
    static void doSendMessage(MyCommandParser::Argument *args, char *response);

    static void listNodes(char *buffer, int hoursLastHeard, bool onlyNeighbors);
    static const _meshtastic_NodeInfoLite *findNode(char *nodeIdOrName);
};

#endif
#endif //MESHTASTIC_MYSLAVESENSOR_H