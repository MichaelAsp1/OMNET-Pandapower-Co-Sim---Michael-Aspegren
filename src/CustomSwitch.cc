#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"
#include <map>

using namespace omnetpp;
using namespace inet;

class CustomSwitch : public cSimpleModule {
  protected:
    std::map<MacAddress, int> macToGate;
    virtual void initialize() override {
        // Map known MAC addresses to output gate indexes.
        // Assuming customSwitch.out[] is defined with 3 elements:
        // Index 0: Controller, Index 1: DER, Index 2: Load.
        macToGate[MacAddress("00:00:00:00:00:02")] = 0; // Controller
        macToGate[MacAddress("00:00:00:00:00:04")] = 1; // DER
        macToGate[MacAddress("00:00:00:00:00:05")] = 2; // Load
    }
    virtual void handleMessage(cMessage *msg) override {
        Packet *pkt = check_and_cast<Packet*>(msg);
        auto ethHeader = pkt->peekAtFront<EthernetMacHeader>();
        MacAddress dest = ethHeader->getDest();

        EV << "CustomSwitch: Received packet for " << dest << "\n";
        auto it = macToGate.find(dest);
        if (it != macToGate.end()) {
            send(pkt, "out", it->second);
            EV << "Forwarding packet out gate " << it->second << "\n";
        }
        else {
            EV_WARN << "Unknown destination MAC: " << dest << ". Dropping packet.\n";
            delete pkt;
        }
    }
};

Define_Module(CustomSwitch);
