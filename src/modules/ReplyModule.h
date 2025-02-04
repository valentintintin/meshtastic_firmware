#ifndef MESHTASTIC_MYSLAVESENSOR_H
#define MESHTASTIC_MYSLAVESENSOR_H

#if USE_REPLYMODULE

#include "Observer.h"
#include "SinglePortModule.h"

#include <CommandParser.h>

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<32,       2,              16,                     200,                0,              200> MyCommandParser;

class ReplyModule : public SinglePortModule, public Observable<const meshtastic_MeshPacket *>
{
  public:
    ReplyModule();

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    void alterReceived(meshtastic_MeshPacket &mp) override;
    meshtastic_MeshPacket *allocReply() override;

private:
    char tempBuffer[MyCommandParser::MAX_RESPONSE_SIZE] = {};
    MyCommandParser parser;

    bool processCommand(const char *command);

    static void doPing(MyCommandParser::Argument *args, char *response);
    static void doNeighbors(MyCommandParser::Argument *args, char *response);
    static void doSearchNeighbor(MyCommandParser::Argument *args, char *response);
    static void doHelp(MyCommandParser::Argument *args, char *response);
};

#endif
#endif //MESHTASTIC_MYSLAVESENSOR_H