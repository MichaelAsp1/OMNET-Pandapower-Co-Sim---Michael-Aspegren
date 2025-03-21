#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"
#include <sstream>
#include <string>

using namespace omnetpp;
using namespace inet;

class MiTM : public cSimpleModule {
  protected:
    virtual void handleMessage(cMessage *msg) override {
        Packet *pkt = check_and_cast<Packet*>(msg);
        EV << "MITM: Intercepted packet " << pkt->getName() << "\n";

        // Check if it's a control packet and modify it
        if (strcmp(pkt->getName(), "ControlSignal") == 0) {
            // Check if the packet has a numeric parameter "controlValue"
            if (pkt->hasPar("controlValue")) {
                double oldVal = pkt->par("controlValue").doubleValue();
                // Example modification: double the value
                double newVal = oldVal + 2.0;
                pkt->par("controlValue").setDoubleValue(newVal);

                // Update the controlData string accordingly
                const char *oldCommand = pkt->par("controlData").stringValue();
                std::ostringstream newCmd;
                // Determine if the command is "Increase" or "Decrease"
                if (strstr(oldCommand, "Increase") != nullptr)
                    newCmd << "Increase DER output by " << newVal << " [MITM modified]";
                else
                    newCmd << "Decrease DER output by " << newVal << " [MITM modified]";

                pkt->par("controlData").setStringValue(newCmd.str().c_str());
                EV << "MITM: Modified controlValue from " << oldVal << " to " << newVal << "\n";
                EV << "MITM: Modified controlData to: " << newCmd.str() << "\n";
            }
            // Forward the modified packet out through gate index 0
            send(pkt, "out", 0);
        } else {
            // For other packets, forward through gate index 1
            send(pkt, "out", 1);
        }
    }
};

Define_Module(MiTM);
