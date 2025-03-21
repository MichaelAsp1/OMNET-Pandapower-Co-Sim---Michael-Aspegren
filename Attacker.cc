#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"

using namespace omnetpp;
using namespace inet;

class Attacker : public cSimpleModule {
  private:
    cMessage *attackTimer = nullptr;  // Timer to trigger a burst of attack packets
    int burstSize;                    // Number of attack packets per burst
    double attackInterval;            // Interval between bursts (in simulation seconds)

  protected:
    virtual void initialize() override {
        // Initialize burst size and attack interval directly in the code.
        burstSize = 10;         // Send 10 attack packets per burst
        attackInterval = 3.0;   // Launch a burst every 3 seconds

        // Create and schedule the attack timer
        attackTimer = new cMessage("attackTimer");
        scheduleAt(simTime() + attackInterval, attackTimer);
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == attackTimer) {
            EV << "Attacker: Launching burst of " << burstSize << " attack packets.\n";
            for (int i = 0; i < burstSize; i++) {
                // Create a bogus attack packet
                Packet *attackPkt = new Packet("AttackPacket");
                auto ethHeader = makeShared<EthernetMacHeader>();
                ethHeader->setChunkLength(B(14));
                // Set source MAC to an attacker address and destination to the controller (or another target)
                ethHeader->setSrc(MacAddress("00:00:00:00:AA:AA"));
                ethHeader->setDest(MacAddress("00:00:00:00:00:02")); // Example target: Controller's MAC
                ethHeader->setTypeOrLength(ETHERTYPE_IPv4);
                attackPkt->insertAtFront(ethHeader);
                // Optionally, add an attack info parameter
                attackPkt->addPar("attackInfo").setStringValue("DoS Attack");

                // Send the attack packet out via the "out" gate
                send(attackPkt, "out");
            }
            // Reschedule the timer for the next burst
            scheduleAt(simTime() + attackInterval, attackTimer);
        }
    }

    virtual void finish() override {
        cancelAndDelete(attackTimer);
    }
};

Define_Module(Attacker);
