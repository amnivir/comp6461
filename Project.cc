/*
 * Project.cc
 *
 *  Created on: Nov 29, 2016
 *      Author: eshinig
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("FTP_Sim");

int main(int argc, char *argv[])
{

    Time::SetResolution (Time::NS);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);


    NodeContainer csmaNodes;

    csmaNodes.Create(2);


    // using csma helper to configure attributes
    // and creating connectivity between two nodes
    CsmaHelper csma;

    // set channel attributes for device
    csma.SetChannelAttribute ("DataRate",StringValue("1Mbps"));
    csma.SetChannelAttribute ("Delay",StringValue("20ms"));



    // installing nodes to create, configure, install devices of required type
    NetDeviceContainer csmaDevices;

    csmaDevices = csma.Install(csmaNodes);

    // installing protocol stack in nodes; once executed will install TCP,UDP, IP etc. on each node of the nodeContainer

    InternetStackHelper stack;
    stack.Install(csmaNodes);

    // assigning IP address to the devices, setting base address here
    Ipv4AddressHelper address;
    address.SetBase ("10.10.10.0","255.255.255.0");


    Ipv4InterfaceContainer csmaInterfaces =address.Assign(csmaDevices);
    // assigning ip address to the devices, since devices contain nodes, while ip stack is created within a node




    //  unint32_t port_num=21; // port number for Rx


    Address RxAddress(InetSocketAddress (csmaInterfaces.GetAddress(1), 10)); //Rx obtaining a IPv4 address, and the port number to listen

    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", RxAddress); // PacketSinkHelper is a class reference for creating socket references and passing address at which Rx is instantiated

    ApplicationContainer sinkApp = sinkHelper.Install (csmaNodes.Get(1)); // ApplicationContainer, installs application on the node, in our case it is packet sink helper ; node here is the server node

    sinkApp.Start (Seconds (1.0));

    sinkApp.Stop (Seconds (10.0));



    //creating a TCP transmitter
    // set up address and port number to  send TCP traffic to the server
    Address TxAddress(InetSocketAddress(csmaInterfaces.GetAddress(1),10));

    OnOffHelper clientHelper ("ns3::TcpSocketFactory", TxAddress);

    // setting up attributes for onoff application helper
    clientHelper.SetAttribute("DataRate",StringValue("1Mbps"));
    clientHelper.SetAttribute("PacketSize",UintegerValue(1280));
    ApplicationContainer Tx = clientHelper.Install (csmaNodes.Get (0));


    csma.EnablePcap("trial",csmaDevices.Get(1),true);
    Tx.Start (Seconds (1.0));
    Tx.Stop (Seconds (10.0));



    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    NS_LOG_INFO ("Done");







    return(0);
}



