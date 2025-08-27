#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/linklayer/common/MacAddress.h"
#include <zmq.hpp>
#include <string>
#include <sstream>

using namespace omnetpp;
using namespace inet;

class MicrogridController : public cSimpleModule {
  private:
    cMessage *requestTimer = nullptr;   // Timer for periodic GET_POWER requests
    cMessage *ackTimer = nullptr;         // Timer for ACK retransmission
    zmq::context_t context{1};
    zmq::socket_t getPowerSocket {context, zmq::socket_type::req};
    zmq::socket_t setDerSocket {context, zmq::socket_type::req};

    // MAC addresses for destination nodes
    MacAddress derMac;
    MacAddress loadMac;

    // PID Controller variables
    double desiredLoad;  // Setpoint (e.g., 0.3 MW)
    double lastError;
    double integral;
    double Kp, Ki, Kd;

    // Sequence number for control packets
    int sequenceNumber;

    // Store the last control packet for potential retransmission
    Packet* lastControlPacket = nullptr;

    // ACK timeout interval (in simulation seconds)
    double ackTimeout = 2.0;

    simsignal_t controlSignalValueSignal;
    simsignal_t controlErrorSignal;
    simsignal_t packetLatencySignal;
    simsignal_t retransmissionCountSignal;

  protected:
    virtual void initialize() override {
        EV << "Initializing MicrogridController...\n";

        // Setup ZeroMQ connection for GET_POWER
        try {
            getPowerSocket.connect("tcp://127.0.0.1:5556");
            EV << "Connected to Pandapower GET_POWER socket.\n";
        }
        catch (const zmq::error_t &e) {
            EV << "ZeroMQ error (GET_POWER): " << e.what() << "\n";
        }
        // Setup ZeroMQ connection for SET_DER_OUTPUT
        try {
            setDerSocket.connect("tcp://127.0.0.1:5557");
            EV << "Connected to Pandapower SET_DER_OUTPUT socket.\n";
        }
        catch (const zmq::error_t &e) {
            EV << "ZeroMQ error (SET_DER_OUTPUT): " << e.what() << "\n";
        }

        // Initialise MAC addresses (ensure these match your network design)
        derMac = MacAddress("00:00:00:00:00:04");
        loadMac = MacAddress("00:00:00:00:00:05");

        // Initialise PID controller parameters
        desiredLoad = 0.3;  // target load in MW
        lastError = 0.0;
        integral = 0.0;
        Kp = 10.0;   // Proportional gain
        Ki = 1.0;    // Integral gain
        Kd = 2.0;    // Derivative gain

        sequenceNumber = 0;

        // Create and schedule timers
        requestTimer = new cMessage("requestTimer");
        scheduleAt(simTime() + 1.0, requestTimer);

        ackTimer = new cMessage("ackTimer");

        controlSignalValueSignal = registerSignal("controlSignalValue");
        controlErrorSignal = registerSignal("controlError");
        packetLatencySignal = registerSignal("packetLatency");
        retransmissionCountSignal = registerSignal("retransmissionCount");
    }

    virtual void handleMessage(cMessage *msg) override {
        if (msg == requestTimer) {
            // Periodically request power data from pandapower
            requestPowerData();
            scheduleAt(simTime() + 5.0, requestTimer);
        }
        else if (strcmp(msg->getName(), "ackTimer") == 0) {
            EV << "ACK timeout reached. Retransmitting last control packet.\n";
            if (lastControlPacket != nullptr) {
                send(lastControlPacket->dup(), "ethg");
                emit(retransmissionCountSignal, 1);
                if (ackTimer->isScheduled())
                    cancelEvent(ackTimer);
                scheduleAt(simTime() + ackTimeout, ackTimer);
            }
        }
        else if (strcmp(msg->getName(), "ACK_ControlSignal") == 0) {
            EV << "Controller received ACK: " << msg->getName() << "\n";
            cancelEvent(ackTimer);
            if (lastControlPacket) {
                double sentTime = lastControlPacket->par("timestamp").doubleValue();
                double delay = simTime().dbl() - sentTime;
                emit(packetLatencySignal, delay);
            }
            delete msg;
            if (lastControlPacket) {
                delete lastControlPacket;
                lastControlPacket = nullptr;
            }
        }
        else {
            Packet *pkt = check_and_cast<Packet*>(msg);
            EV << "Controller received a packet: " << pkt->getName() << "\n";
            delete pkt;
        }
    }

    void requestPowerData() {
        try {
            // Send GET_POWER request...
            std::string request = "GET_POWER";
            zmq::message_t message(request.size());
            memcpy(message.data(), request.c_str(), request.size());
            getPowerSocket.send(message, zmq::send_flags::none);
            EV << "Sent GET_POWER request to pandapower.\n";

            zmq::message_t reply;
            getPowerSocket.recv(reply, zmq::recv_flags::none);
            std::string powerData = reply.to_string();
            EV << "Received from pandapower: " << powerData << "\n";

            double measuredLoad = std::stod(powerData);
            EV << "Parsed measured load: " << measuredLoad << " MW\n";

            // Compute control signal based on PID:
            double error = desiredLoad - measuredLoad;
            integral += error * 5.0;  // assuming 5 s between updates
            double derivative = (error - lastError) / 5.0;
            double controlSignal = Kp * error + Ki * integral + Kd * derivative;
            lastError = error;

            emit(controlErrorSignal, error);
            emit(controlSignalValueSignal, controlSignal);

            // Instead of sending DER update only if controlSignal > 0,
            // compute the desired DER output directly.
            // For example, you might use:
            double desiredDEROutput = fabs(controlSignal);  // or some function of the error
            std::ostringstream cmdStream;
            if (controlSignal > 0)
                cmdStream << "Increase DER output by " << desiredDEROutput;
            else
                cmdStream << "Decrease DER output by " << desiredDEROutput;
            std::string controlCommand = cmdStream.str();

            // Always send the DER update:
            sendControlPacket(controlCommand, true); // true indicates DER command

            // Send the new DER setpoint to pandapower via SET_DER_OUTPUT command
            std::ostringstream setCmd;
            setCmd << "SET_DER_OUTPUT " << desiredDEROutput;
            std::string setCommand = setCmd.str();
            zmq::message_t setMsg(setCommand.size());
            memcpy(setMsg.data(), setCommand.c_str(), setCommand.size());
            setDerSocket.send(setMsg, zmq::send_flags::none);
            EV << "Sent SET_DER_OUTPUT command to pandapower: " << setCommand << "\n";

            zmq::message_t setReply;
            setDerSocket.recv(setReply, zmq::recv_flags::none);
            std::string setAck = setReply.to_string();
            EV << "Received DER update ack from pandapower: " << setAck << "\n";
        }
        catch (const zmq::error_t &e) {
            EV << "ZeroMQ error: " << e.what() << "\n";
        }
        catch (const std::exception &ex) {
            EV << "Error parsing power data: " << ex.what() << "\n";
        }
    }

    void sendControlPacket(const std::string &controlData, bool toDER) {
        Packet *packet = new Packet("ControlSignal");

        auto ethHeader = makeShared<EthernetMacHeader>();
        ethHeader->setChunkLength(B(14));
        ethHeader->setSrc(MacAddress("00:00:00:00:00:02"));
        ethHeader->setDest(toDER ? derMac : loadMac);
        ethHeader->setTypeOrLength(ETHERTYPE_IPv4);

        packet->addPar("seq").setLongValue(sequenceNumber);
        packet->addPar("timestamp").setDoubleValue(simTime().dbl());
        packet->addPar("controlData").setStringValue(controlData.c_str());

        // Extract numeric value from the control command string.
        double controlValue = 0.0;
        {
            std::istringstream iss(controlData);
            std::string dummy;
            for (int i = 0; i < 4 && iss >> dummy; i++); // Skip tokens "Increase/Decrease", "DER", "output", "by"
            iss >> controlValue;
        }
        packet->addPar("controlValue").setDoubleValue(controlValue);

        packet->insertAtFront(ethHeader);

        EV << "Controller sent control packet (" << controlData << ") to "
           << (toDER ? "DER" : "Load") << " with seq " << sequenceNumber << "\n";

        if (lastControlPacket)
            delete lastControlPacket;
        lastControlPacket = packet->dup();

        send(packet, "ethg");

        sequenceNumber++;

        if (ackTimer->isScheduled())
            cancelEvent(ackTimer);
        scheduleAt(simTime() + ackTimeout, ackTimer);
    }

  public:
    ~MicrogridController() {
        getPowerSocket.close();
        setDerSocket.close();
        cancelAndDelete(requestTimer);
        if (ackTimer->isScheduled())
            cancelEvent(ackTimer);
        delete ackTimer;
        if (lastControlPacket)
            delete lastControlPacket;
    }
};

Define_Module(MicrogridController);
