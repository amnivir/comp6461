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
// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 10.1.3.0
//                 AP<--------No. of access points can be changed between 1-15 : check nWifi
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n4   n3   n2   n0 -------------- n1  <-----ftp server
//                   point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
    uint32_t maxBytes = 0;
    //uint32_t nCsma = 1; //only  n2  exist
    uint32_t nWifi = 1 ;

    // Check for valid number of csma or wifi nodes
    // 250 should be enough, otherwise IP addresses
    // soon become an issue
    if (nWifi > 250 )
    {
        std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
        return 1;
    }

    NodeContainer AP_FTP_Nodes;
    AP_FTP_Nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    //    NetDeviceContainer p2pDevices;
    //    p2pDevices = pointToPoint.Install (AP_FTP_Nodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);
    NodeContainer wifiApNode = AP_FTP_Nodes.Get (0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");
    mac.SetType ("ns3::StaWifiMac",
            "Ssid", SsidValue (ssid),
            "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
            "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);

    MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
            "MinX", DoubleValue (0.0),
            "MinY", DoubleValue (0.0),
            "DeltaX", DoubleValue (5.0),
            "DeltaY", DoubleValue (10.0),
            "GridWidth", UintegerValue (3),
            "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
            "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
    mobility.Install (wifiStaNodes);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);

    //////////////////TOP LAN////////////////////
    Ptr<Node> n2 = CreateObject<Node> ();//acts as a router


    Ptr<Node> bridge1 = CreateObject<Node> ();
    Ptr<Node> bridge2 = CreateObject<Node> ();

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("1Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

    NetDeviceContainer topLanDevices;
    NetDeviceContainer topBridgeDevices;
    // It is easier to iterate the nodes in C++ if we put them into a container
    NodeContainer topLan (n2);
    topLan.Add(AP_FTP_Nodes.Get(0));
    topLan.Add(wifiStaNodes);


    for (uint32_t i = 0; i < (nWifi+2); i++)
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
    //stack.Install (csmaNodes);
    stack.Install (AP_FTP_Nodes);
    //stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);
    stack.Install(n2);



/////////////BOTTOM LAN??????????????
    NetDeviceContainer bottomLanDevices;
    NetDeviceContainer bottomBridgeDevices;
    NodeContainer bottomLan (n2);
    bottomLan.Add(AP_FTP_Nodes.Get(1));
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
        BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(APInterfaces.GetAddress(i+2), port));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
        ApplicationContainer sourceApps = source.Install(AP_FTP_Nodes.Get(1));
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(10.0));

        std::cout<<"CLIENT_IP: "<<APInterfaces.GetAddress(i+2)<<"\n";
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
    Simulator::Run ();

    // 10. Print per flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
            i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
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
                / (i->second.timeLastRxPacket.GetSeconds()
                        - i->second.timeFirstTxPacket.GetSeconds()) / 1024 << " Kbps \n";
        std::cout << "  Delay:      " << i->second.delaySum / i->second.rxPackets  <<"\n";
    }
    monitor->SerializeToXmlFile("Project6461.xml", true, true);
    Simulator::Destroy ();
    return 0;
}
