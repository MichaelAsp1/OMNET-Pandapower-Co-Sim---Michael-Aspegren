#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"

using namespace omnetpp;
using namespace inet;

class Load : public cSimpleModule {
  protected:
    virtual void initialize() override {
        EV << "LoadNode initialized.\n";
    }

    virtual void handleMessage(cMessage *msg) override {
        Packet *pkt = check_and_cast<Packet*>(msg);
        EV << "LoadNode received a control signal: " << pkt->getName() << "\n";
        delete pkt;

        // Create an acknowledgment packet with Ethernet header.
        Packet *ackPkt = new Packet("ACK_ControlSignal");

        // Create and configure the Ethernet header.
        auto ethHeader = makeShared<EthernetMacHeader>();
        ethHeader->setChunkLength(B(14));
        // Set source MAC to Load's MAC (example MAC for Load)
        ethHeader->setSrc(MacAddress("00:00:00:00:00:05"));
        // Set destination MAC to Controller's MAC (example MAC for Controller)
        ethHeader->setDest(MacAddress("00:00:00:00:00:02"));
        // Set Ethernet type (e.g., IPv4, using the standard value)
        ethHeader->setTypeOrLength(ETHERTYPE_IPv4);

        // Insert header at the front of the packet.
        ackPkt->insertAtFront(ethHeader);

        // Send the acknowledgment packet. The custom switch will read the dest MAC.
        send(ackPkt, "out");
    }
};

Define_Module(Load);
