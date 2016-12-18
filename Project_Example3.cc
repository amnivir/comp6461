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
//  |    |    |    |   10.1.1.0
// n4   n3   n2   n0 ------SW1----SW2---- n1  <-----ftp server
//                   CSMA

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThirdScriptExample");

int main(int argc, char *argv[])
{
    uint32_t maxBytes = 0;
    bool verbose = true;
    //uint32_t nCsma = 1; //only  n2  exist
    uint32_t nWifi = 4;
    bool tracing = false;

    CommandLine cmd;
    //cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    // Check for valid number of csma or wifi nodes
    // 250 should be enough, otherwise IP addresses
    // soon become an issue
    if (nWifi > 15)
    {
        std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
        return 1;
    }

    NodeContainer csmaNodes;
    csmaNodes.Create(2);

    //SWITCH
    NodeContainer csmaSwitch;
    csmaSwitch.Create(2);

    PointToPointHelper pointToPointSwitches;
    pointToPointSwitches.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPointSwitches.SetChannelAttribute("Delay", StringValue("2ms"));

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    NetDeviceContainer p2pSwitches;
    //p2pSwitches = pointToPointSwitches.Install(csmaSwitch);
    p2pSwitches=csma.Install(csmaSwitch);
    NetDeviceContainer csmaDevices;
    //csmaDevices = csma.Install (csmaNodes);

    NetDeviceContainer switchDevices1;
    NetDeviceContainer switchDevices2;

    //for (int i = 0; i < 2; i++)

    NetDeviceContainer link = csma.Install(NodeContainer(csmaNodes.Get(0), csmaSwitch.Get(0)));
    csmaDevices.Add(link.Get(0));
    switchDevices1.Add(link.Get(1));

    link = csma.Install(NodeContainer(csmaNodes.Get(1), csmaSwitch.Get(1)));
    csmaDevices.Add(link.Get(0));
    switchDevices2.Add(link.Get(1));

    // Create the bridge netdevice, which will do the packet switching
    Ptr<Node> switchNode = csmaSwitch.Get(0);
    BridgeHelper bridge1;
    bridge1.Install(switchNode, switchDevices1);

    Ptr<Node> switchNode2 = csmaSwitch.Get(1);
    BridgeHelper bridge2;
    bridge2.Install(switchNode2, switchDevices2);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = csmaNodes.Get(0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator", "MinX", DoubleValue(0.0), "MinY",
            DoubleValue(0.0), "DeltaX", DoubleValue(5.0), "DeltaY", DoubleValue(10.0), "GridWidth",
            UintegerValue(3), "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds",
            RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    InternetStackHelper stack;
    stack.Install(csmaSwitch);
    stack.Install(csmaNodes.Get(1));
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(csmaDevices);

    //    address.SetBase ("10.1.2.0", "255.255.255.0");
    //    Ipv4InterfaceContainer csmaInterfaces;
    //    csmaInterfaces = address.Assign (csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiInterfaces;
    wifiInterfaces = address.Assign(staDevices);
    address.Assign(apDevices);

    //
    // Create a BulkSendApplication and install it on node 0, FTP TCP server
    //      //
    uint16_t port = 9;  // well-known echo port number
    std::cout << "FTP_SERVER_IP: " << p2pInterfaces.GetAddress(1) << "\n";
    for (uint32_t i = 0; i < nWifi; i++)
    {
        BulkSendHelper source("ns3::TcpSocketFactory",
                InetSocketAddress(wifiInterfaces.GetAddress(i), port));
        // Set the amount of data to send in bytes.  Zero is unlimited.
        source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
        ApplicationContainer sourceApps = source.Install(csmaNodes.Get(1));
        sourceApps.Start(Seconds(0.0));
        sourceApps.Stop(Seconds(10.0));

        std::cout << "CLIENT_IP: " << wifiInterfaces.GetAddress(i) << "\n";
        PacketSinkHelper sink("ns3::TcpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApps = sink.Install(wifiStaNodes.Get(i));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(10.0));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::Stop(Seconds(10.0));

    //    if (tracing == true)
    //    {
    //        pointToPoint.EnablePcapAll ("third");
    //        phy.EnablePcap ("third", apDevices.Get (0));
    //        csma.EnablePcap ("third", csmaDevices.Get (0), true);
    //    }

    Simulator::Run();

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
        std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 10.0 / 1000 / 1000 << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: "
                << i->second.rxBytes * 8.0
                        / (i->second.timeLastRxPacket.GetSeconds()
                                - i->second.timeFirstRxPacket.GetSeconds()) / 1024 << " Kbps \n";
        std::cout << "  Delay:      " << i->second.delaySum / i->second.rxPackets << "\n";
    }
    monitor->SerializeToXmlFile("Project6461.xml", true, true);
    Simulator::Destroy();
    return 0;
}
