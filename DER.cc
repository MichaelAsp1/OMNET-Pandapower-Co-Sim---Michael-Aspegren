#include <omnetpp.h>

using namespace omnetpp;

class DER : public cSimpleModule
{
  protected:
    virtual void initialize() override {
        EV << "DerNode initialized.\n";
    }

    virtual void handleMessage(cMessage *msg) override {
        EV << "DerNode received a message: " << msg->getName() << "\n";
        // For now, simply delete the incoming message
        delete msg;
    }
};

Define_Module(DER);
