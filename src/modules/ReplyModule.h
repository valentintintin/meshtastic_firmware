#pragma once
#include "Observer.h"
#include "SinglePortModule.h"

class ReplyModule : public SinglePortModule, public Observable<const meshtastic_MeshPacket *>
{
  public:
    ReplyModule() : SinglePortModule("reply", meshtastic_PortNum_TEXT_MESSAGE_APP) {}

  protected:
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;
    virtual void alterReceived(meshtastic_MeshPacket &mp) override;
    virtual meshtastic_MeshPacket *allocReply() override;

private:
    char tempBuffer[200] = { '\0' };
};
