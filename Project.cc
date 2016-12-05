/*
 * Project.cc
 *
 *  Created on: Nov 29, 2016
 *      Author: eshinig
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/propagation-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FTP_Sim");
/**
 * Topology
 *
 * Node 1 (Client) ----CSMA--- Node 2 (Server)
 */
int main(int argc, char *argv[])
{

    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    //bool tracing = true;
    uint32_t maxBytes = 0;
    //
    // Allow the user to override any of the defaults at
    // run-time, via command-line arguments
    //
    /*CommandLine cmd;
     cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
     cmd.AddValue ("maxBytes",
     "Total number of bytes for application to send", maxBytes);
     cmd.Parse (argc, argv);
     */
    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Create channels.");

    //
    // Explicitly create the point-to-point link required by the topology (shown above).
    //
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("500Kbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    //
    // Install the internet stack on the nodes
    //
    InternetStackHelper internet;
    internet.Install(nodes);

    //
    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    NS_LOG_INFO("Create Applications.");

    //
    // Create a BulkSendApplication and install it on node 0, FTP TCP server
    //
    uint16_t port = 9;  // well-known echo port number

    //send to node 1
    std::cout<<"FTP_SERVER_IP"<<i.GetAddress(0)<<"\n";
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(i.GetAddress(1), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(10.0));

    //
    // Create a PacketSinkApplication and install it on node 1
    //
    std::cout<<"CLIENT_IP"<<i.GetAddress(1)<<"\n";
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    //
    // Set up tracing if enabled
    //
    /*
     //    if (tracing)
     {
     AsciiTraceHelper ascii;
     pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
     pointToPoint.EnablePcapAll ("tcp-bulk-send", true);
     }
     */
    //collect reults
    // Flow monitor
    /* Ptr<FlowMonitor> flowMonitor;
     FlowMonitorHelper flowHelper;
     flowMonitor = flowHelper.InstallAll();*/

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(10.0));
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
        std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000
                << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: "
                << i->second.rxBytes * 8.0
                / (i->second.timeLastRxPacket.GetSeconds()
                        - i->second.timeFirstTxPacket.GetSeconds()) / 1024 << " Kbps";
        std::cout << "  Delay:      " << i->second.delaySum / 1000 / 1000 / 1000 << " Sec.\n";
    }
    monitor->SerializeToXmlFile("Project6461.xml", true, true);
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    //flowMonitor->SerializeToXmlFile("ftp_test.xml", true, true);

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Received at client: " << sink1->GetTotalRx () << std::endl;

}

