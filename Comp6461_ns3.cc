/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/propagation-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-module.h"
// COMP 6461 Network Topology
//
// Number of wifi or csma nodes can be increased up to 150
//                          |
// WifiUsers(n1-n4) 10.1.2.0
//                              AccessPoint
//  *    *    *   *               *                  *              *             *           *
//  |    |    |    |           10.1.2.1            10.1.1.0                      10.1.3.0     10.1.3.2
// n4   n3   n2   n1             n0--------------  SWITCH_1--------Router-------SWITCH_2-----FTP TCP Server
//

using namespace ns3;

//static void
//SetPosition (Ptr<Node> node, double x)
//{
//    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
//    Vector pos = mobility->GetPosition();
//    pos.x = x;
//    mobility->SetPosition(pos);
//}
NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int
main (int argc, char *argv[])
{
    uint32_t maxBytes = 1024 * 100;// 0.5MB

    uint32_t nWifi = 4;
    // Check for valid number of csma or wifi nodes
    // 250 should be enough, otherwise IP addresses
    // soon become an issue
    if (nWifi > 150 )
    {
        std::cout << "Too many wifi or csma nodes, no more than 15 each." << std::endl;
        return 1;
    }

    LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);

    NodeContainer FTP_NODE;
    FTP_NODE.Create(1);
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());
    WifiHelper wifi;// = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

    // Add a non-QoS upper mac

    Ssid ssid1 = Ssid ("wifi-ap1");
    mac.SetType ("ns3::StaWifiMac",
            "Ssid", SsidValue (ssid1),
            "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);


    //QosWifiMacHelper wifiMacHelper = QosWifiMacHelper::Default ();
    mac.SetType ("ns3::ApWifiMac",
            "Ssid", SsidValue (ssid1),
            "BeaconGeneration", BooleanValue (true),
            "BeaconInterval", TimeValue (Seconds (2.5)));
    //    mac.SetType("ns3::ApWifiMac",
    //            "Ssid",SsidValue(ssid1));
    NetDeviceContainer apDevices;

    apDevices = wifi.Install(phy,mac,wifiApNode.Get(0));
    //phy.SetChannel (channel.Create ());
    //apDevices.Add(wifi.Install(phy,mac,wifiApNode.Get(1)));

    MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
            "MinX", DoubleValue (0.0),
            "MinY", DoubleValue (0.0),
            "DeltaX", DoubleValue (5.0),
            "DeltaY", DoubleValue (10.0),
            "GridWidth", UintegerValue (30),
            "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
            "Bounds", RectangleValue (Rectangle (-150, 150, -150, 150)));
    mobility.Install (wifiStaNodes);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode.Get(0));

    // node 1 comes in the communication range of both
    //    Simulator::Schedule (Seconds (4.0), &SetPosition, wifiStaNodes.Get (0), 200.0);

    // node 1 goes out of the communication range of both
    //Simulator::Schedule (Seconds (4.0), &SetPosition, wifiStaNodes.Get (0), 115.0);

    //////////////////TOP LAN////////////////////
    Ptr<Node> n2 = CreateObject<Node> ();//acts as a router
    Ptr<Node> bridge1 = CreateObject<Node> ();
    Ptr<Node> bridge2 = CreateObject<Node> ();

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("2Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    NetDeviceContainer topLanDevices;
    NetDeviceContainer topBridgeDevices;
    // It is easier to iterate the nodes in C++ if we put them into a container
    NodeContainer topLan;
    topLan.Add(n2);
    topLan.Add(wifiApNode);

    for (uint32_t i = 0; i < 2; i++) //TODO make this constant
    {
        // install a csma channel between the ith toplan node and the bridge node
        NetDeviceContainer link = csma.Install (NodeContainer (topLan.Get (i), bridge1));
        topLanDevices.Add (link.Get (0));
        topBridgeDevices.Add (link.Get (1));
    }

    //
    // Now, Create the bridge netdevice, which will do the packet switching.  The
    // bridge lives on the node bridge1 and bridges together the topBridgeDevices
    // which are the three CSMA net devices on the node in the diagram above.
    //
    BridgeHelper bridge;
    bridge.Install (bridge1, topBridgeDevices);

    //TODO change here, add
    InternetStackHelper stack;

    stack.Install (wifiApNode);
    stack.Install(FTP_NODE);
    stack.Install (wifiStaNodes);
    stack.Install(n2);


    NetDeviceContainer topLanDevices1;
    topLanDevices1.Add(staDevices);
    topLanDevices1.Add(apDevices);


    /////BOTTOM LAN/////////////////
    NetDeviceContainer bottomLanDevices;
    NetDeviceContainer bottomBridgeDevices;
    NodeContainer bottomLan (n2);
    bottomLan.Add(FTP_NODE.Get(0));
    for (int i = 0; i < 2; i++)
    {
        NetDeviceContainer link = csma.Install (NodeContainer (bottomLan.Get (i), bridge2));
        bottomLanDevices.Add (link.Get (0));
        bottomBridgeDevices.Add (link.Get (1));
    }
    bridge.Install (bridge2, bottomBridgeDevices);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer APInterfaces;
    APInterfaces = address.Assign (topLanDevices);

    Ipv4InterfaceContainer wifiClients;
    address.SetBase ("10.1.2.0", "255.255.255.0");
    wifiClients = address.Assign (topLanDevices1);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer bottomInterfaces;
    bottomInterfaces = address.Assign (bottomLanDevices);
    //address.Assign (apDevices);

    //
    // Create a BulkSendApplication and install it on node 0, FTP TCP server
    //      //
    uint16_t port = 9;  // well-known echo port number
    std::cout<<"FTP_SERVER_IP: "<<bottomInterfaces.GetAddress(1)<<"\n";
    for(uint32_t i=0;i<nWifi;i++)
    {
        BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(wifiClients.GetAddress(i), port));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
        ApplicationContainer sourceApps = source.Install(FTP_NODE.Get(0));

        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(10.0));

        std::cout<<"CLIENT_IP: "<<wifiClients.GetAddress(i)<<"\n";
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = sink.Install(wifiStaNodes.Get(i));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(10.0));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::Stop (Seconds (10.0));

    phy.EnablePcapAll ("trace", true);
    Simulator::Run ();

    // 10. Print per flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
            i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        if(t.destinationAddress=="10.1.2.1")
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
