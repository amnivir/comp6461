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
// n4   n3   n2   n0 ------SWITCH_1-------- n1  <-----ftp server
//                   ...CSMA

using namespace ns3;

static void
SetPosition (Ptr<Node> node, double x)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition();
  pos.x = x;
  mobility->SetPosition(pos);
}
NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int
main (int argc, char *argv[])
{
    uint32_t maxBytes = 1024;
    //uint32_t nCsma = 1; //only  n2  exist

    uint32_t nWifi = 1;
    std::string phyMode ("DsssRate1Mbps");


    // Check for valid number of csma or wifi nodes
    // 250 should be enough, otherwise IP addresses
    // soon become an issue
    if (nWifi > 15 )
    {
        std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
        return 1;
    }


    LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);

    NodeContainer AP_FTP_Nodes;
    AP_FTP_Nodes.Create (2);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);
    NodeContainer wifiApNode = AP_FTP_Nodes.Get (0);

    // Set up WiFi
    WifiHelper wifi;

    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel ;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
                                      "SystemLoss", DoubleValue(1),
                                      "HeightAboveZ", DoubleValue(1.5));

    // For range near 250m
    wifiPhy.Set ("TxPowerStart", DoubleValue(33));
    wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
    wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
    wifiPhy.Set ("TxGain", DoubleValue(0));
    wifiPhy.Set ("RxGain", DoubleValue(0));
    wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-61.8));
    wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-64.8));


    wifiPhy.SetChannel (wifiChannel.Create ());

    // Add a non-QoS upper mac
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");

    // Set 802.11b standard
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue(phyMode),
                                  "ControlMode",StringValue(phyMode));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (wifiPhy, wifiMac, wifiStaNodes);


    NetDeviceContainer apDevices;
    apDevices = wifi.Install (wifiPhy, wifiMac, wifiApNode);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);

    MobilityHelper mobilityUser;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
    positionAlloc ->Add(Vector(1000000000, 0, 0)); // node1 -- starting very far away
    mobilityUser.SetPositionAllocator(positionAlloc);
    mobilityUser.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityUser.Install(wifiStaNodes);


    // node 1 comes in the communication range of both
    Simulator::Schedule (Seconds (4.0), &SetPosition, wifiStaNodes.Get (0), 200.0);

    // node 1 goes out of the communication range of both
    Simulator::Schedule (Seconds (7.0), &SetPosition, wifiStaNodes.Get (0), 1000.0);


    //////////////////TOP LAN////////////////////
    Ptr<Node> n2 = CreateObject<Node> ();//acts as a router


    Ptr<Node> bridge1 = CreateObject<Node> ();
    Ptr<Node> bridge2 = CreateObject<Node> ();

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
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

    stack.Install (AP_FTP_Nodes);

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
                        - i->second.timeFirstRxPacket.GetSeconds()) / 1024 << " Kbps \n";
        std::cout << "  Delay:      " << i->second.delaySum / i->second.rxPackets  <<"\n";
    }
    monitor->SerializeToXmlFile("Project6461.xml", true, true);
    Simulator::Destroy ();
    return 0;
}
