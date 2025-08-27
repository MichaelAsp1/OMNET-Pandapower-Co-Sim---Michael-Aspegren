package zeromqclient.simulations;

simple MicrogridController
{
    gates:
        input in;
        output ethg;   // renamed to match the code
}

simple CustomSwitch
{
    gates:
        input in[3];
        output out[3];  // declared as a vector for multiple connections
}

simple DER
{
    gates:
        input in;
        output out;
}

simple Load
{
    gates:
        input in;
        output out;
}

//simple Attacker {
  //  gates:
	//output out;
//}

//simple MiTM
//{
  //  gates:
    //    input in[2];
      //  output out[2];
//}

network MicrogridNetwork
{
    @display("bgb=578,354");
    submodules:
        controller: MicrogridController {
            @display("p=300,150");
        }
        customSwitch: CustomSwitch {
            @display("i=device/switch");
        }
        der: DER {
            @display("p=200,250");
        }
        load: Load {
            @display("p=200,50");
        }
        //attacker: Attacker {
          //  @display("i=device/virus, p=300,50");
        //}
        //mitm: MiTM {
          //  @display("i=block/caution, p=150,150");
        //}
    connections allowunconnected:
        controller.ethg --> customSwitch.in++;
        customSwitch.out++ --> controller.in;
        customSwitch.out++ --> der.in;
        der.out --> customSwitch.in++;
        customSwitch.out++ --> load.in;
        load.out --> customSwitch.in++;
        //attacker.out --> customSwitch.in++;

        //der.out --> mitm.in++;
        //customSwitch.out++ --> mitm.in++;
        //mitm.out++ --> der.in;
        //mitm.out++ --> customSwitch.in++;

}
