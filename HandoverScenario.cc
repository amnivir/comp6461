/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
// Default Network Topology
//
// Wifi 10.1.1.0
// AP
// * *
// | |
// n1 n0
//
//Consider a Wifi access point (AP) and a base station (STA), which are both static.
//The STA can communicate with the AP only when it is within a certain distance
//from the AP. Beyond that range, the two nodes can't receive each others signals.
//
// Given below is a code to simulate the said scenario with ns3.
// STA sends a packet to the AP; AP echoes it back to the base station.
// The AP is located at position (x, y) = (0, 0). STA is at (xDistance, 0)
// (all distances in meter). Default value of xDistance is set to 10. [Lines #76, #131]
//
//  Increase the value of xDistance in the code and find out the maximum distance upto which two way communication is possible. This can be verified from the output of the code, which will show the STA has received reply from the AP (indicated by IP address).
// Node #0 is the AP, #1 is a base station
// #1 sends UDP echo mesg to the AP; AP sends a UDP response back to the node
// Communication is possible only when the station is within a certain distance from the AP

// Mobility model is used for calculating propagation loss and propagation delay.
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/propagation-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Wifi-2-nodes-fixed");
void
PrintLocations (NodeContainer nodes, std::string header)
{
    std::cout << header << std::endl;
    for(NodeContainer::Iterator iNode = nodes.Begin (); iNode != nodes.End (); ++iNode)
    {
        Ptr<Node> object = *iNode;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        NS_ASSERT (position != 0);
        Vector pos = position->GetPosition ();
        std::cout << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    std::cout << std::endl;
}
void
PrintAddresses(Ipv4InterfaceContainer container, std::string header)
{
    std::cout << header << std::endl;
    uint32_t nNodes = container.GetN ();
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        std::cout << container.GetAddress(i, 0) << std::endl;
    }
    std::cout << std::endl;
}

static void
SetPosition (Ptr<Node> node, double x)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    Vector pos = mobility->GetPosition();
    pos.x = x;
    mobility->SetPosition(pos);
}

int
main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nWifi = 2;
    /** Change this parameter and verify the output */
    double xDistance = 10;

    CommandLine cmd;
    cmd.AddValue ("xDistance", "Distance between two nodes along x-axis", xDistance);

    cmd.Parse (argc,argv);
    if (verbose)
    {
        //        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        //        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
    }
    // 1. Create the nodes and hold them in a container
    NodeContainer wifiStaNodes, wifiApNode;

    wifiStaNodes.Create (nWifi);
    wifiApNode.Create(2);
    // 2. Create channel for communication
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());
    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
    // 3a. Set up MAC for base stations

    // 3b. Set up MAC for AP1 & CLient & Server

    Ssid ssid1 = Ssid ("wifi-ap1");
    mac.SetType ("ns3::StaWifiMac",
            "Ssid", SsidValue (ssid1),
            "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices1;
    staDevices1 = wifi.Install(phy, mac, wifiStaNodes);

    NetDeviceContainer apDevice;
    mac.SetType("ns3::ApWifiMac",
            "Ssid",SsidValue(ssid1));
    apDevice= wifi.Install(phy,mac,wifiApNode);

    // 3b. Set up MAC for AP2

    // 4. Set mobility of the nodes
    MobilityHelper mobility;

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (-100.0, 0, 0.0));//0-->AP1
    positionAlloc->Add (Vector (100.0, 0, 0.0));//0-->AP2
    positionAlloc->Add (Vector (-115.0, 0.0, 0.0));//1-->client
    positionAlloc->Add (Vector (-110.0, 0.0, 0.0));//2-->Server
    //positionAlloc->Add (Vector (0.0, 115.0, 0.0));//3
    //positionAlloc->Add (Vector (0.0, -150.0, 0.0));//4
    mobility.SetPositionAllocator (positionAlloc);


    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);
    mobility.Install (wifiStaNodes);

    // node 1 comes in the communication range of both
    Simulator::Schedule (Seconds (5.0), &SetPosition, wifiStaNodes.Get (1), 110.0);
    // node 1 goes out of the communication range of both
    Simulator::Schedule (Seconds (5.0), &SetPosition, wifiStaNodes.Get (0), 115.0);
    // 5.Add Internet layers stack
    InternetStackHelper stack;
    stack.Install (wifiStaNodes);
    stack.Install (wifiApNode);
    // 6. Assign IP address to each device
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer wifiInterfaces;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    wifiInterfaces = address.Assign(staDevices1);
    address.Assign(apDevice);
    // 7a. Create and setup applications (traffic sink)
    //    UdpEchoServerHelper echoServer (9); // Port # 9
    //    ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get(1));
    //    serverApps.Start (Seconds (1.0));
    //    serverApps.Stop (Seconds (9.0));
    //
    ////    UdpEchoServerHelper echoServer1 (10); // Port # 10
    ////    ApplicationContainer serverApps1 = echoServer1.Install (wifiStaNodes.Get(3));
    ////    serverApps1.Start (Seconds (6.0));
    ////    serverApps1.Stop (Seconds (9.0));
    //
    //    // 7b. Create and setup applications (traffic source)
    //    UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (1), 9);
    //    echoClient.SetAttribute ("MaxPackets", UintegerValue (50));
    //    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    //    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    //
    //    ApplicationContainer clientApps = echoClient.Install (wifiStaNodes.Get (0));
    //    clientApps.Start (Seconds (2.0));
    //    clientApps.Stop (Seconds (10.0));

    uint16_t port = 9;  // well-known echo port number
    std::cout<<"FTP_SERVER_IP: "<<wifiInterfaces.GetAddress(1)<<"\n";


    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(wifiInterfaces.GetAddress(0), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApps = source.Install(wifiStaNodes.Get(1));

    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(10.0));

    std::cout<<"CLIENT_IP: "<<wifiInterfaces.GetAddress(0)<<"\n";
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(wifiStaNodes.Get(0));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));


    //    ApplicationContainer clientApps1 = echoClient1.Install (wifiStaNodes.Get (2));
    //    clientApps1.Start (Seconds (7.0));
    //    clientApps1.Stop (Seconds (8.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::Stop (Seconds (10.0));
    // 8. Enable tracing (optional)
    phy.EnablePcapAll ("wifi-2-nodes-fixed", true);

    PrintAddresses(wifiInterfaces, "IP addresses of base stations");
    PrintLocations(wifiStaNodes, "Location of all nodes");

    Simulator::Run ();
    // 10. Print per flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
            i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        {
            std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                    << t.destinationAddress << ")\n";
            std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 10.0 / 1000 / 1000
                    << " Mbps\n";
            std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "  Throughput: "
                    << i->second.rxBytes * 8.0
                    / 10 / 1024 << " Kbps \n";
            std::cout << "  Delay:      " << i->second.delaySum / i->second.rxPackets  <<"\n";
            std::cout <<" Lost Packets  " << i->second.lostPackets<<"\n";
            std::cout <<" Packets Dropped " << i->second.packetsDropped.size()<<"\n";
        }
    }
    monitor->SerializeToXmlFile("Simulation_Data.xml", true, true);

    Simulator::Destroy ();

    return 0;
}
