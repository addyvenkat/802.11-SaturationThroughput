/*
Authors: ADITHYA VENKAT RAMANAN (USC ID:1932332235)
	 SIRISHA K S (USC ID:3251982100)
   
As part of our EE597 curriculum, this project aims to validate 
the analytical model by Bianchi [1] for computing the saturation
throughput performance of the 802.11 protocol.

This NS3 code is mostly referred from the tutorial documentation [2]
and other references mentioned at the end of the code.

The topology used is as follows (inspired from [3]):
	.
	.
	X
	|    O: Access point
	X    X: Station
	|
	X
	|
..X-X-X-O-X-X-X..
	|
	X
	|
	X
	|
	X
	.
	.
*/


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"
#include <fstream>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ee597-projectlog");

double simTime = 10.0;
std::string filename = "throughput-vs-#stations";
std::string graphicsFilename = filename+".png";
std::string plotFilename = filename+".plt";
std::string plotTitle = "IEEE802.11 - Saturation Throughput";
std::string dataTitle = "Throughput";

int
main(int argc, char *argv[])
{
  // Initializing gnuplot (referred from [4])
  Gnuplot plot (graphicsFilename);
  plot.SetTitle (plotTitle);
  plot.SetTerminal ("png");
  plot.SetLegend ("Number of stations","Throughput");
  Gnuplot2dDataset dataset;
  dataset.SetTitle (dataTitle);
  dataset.SetStyle (Gnuplot2dDataset::LINES);
  uint16_t mcs = 7;
  
  // Default Inputs
  uint32_t Wo = 32;
  uint32_t m = 3;

  // Inputs from Command line
  CommandLine cmd;
  cmd.AddValue ("Wo", "Minimum backoff window", Wo);
  cmd.AddValue ("m", "Maximum stage", m);
  cmd.Parse (argc, argv);

for(uint32_t nWifi=2;nWifi<=50;nWifi++)
{	
  // Setting the values of the backoff/contention window
  Config::SetDefault ("ns3::Txop::MinCw", UintegerValue (Wo-1));
  Config::SetDefault ("ns3::Txop::MaxCw", UintegerValue ((pow(2,m)*Wo)-1));

  // Station Nodes and AP node (referred from tutorial [2])
  NodeContainer wifiStaNodes; 
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode;
  wifiApNode.Create(uint32_t(1));

  // PHY Layer (referred from tutorial [2])
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default (); 
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  // MAC Layer (referred from [5])
  WifiMacHelper mac;
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  std::ostringstream oss;
  oss << "HtMcs" << mcs;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (oss.str ()),
                                "ControlMode", StringValue (oss.str ()));


  Ssid ssid = Ssid ("ns3-80211n");

  mac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid));
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);
  
  
  // Position for Stations and AP (model referred from [6])
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  float d = 1;
  uint32_t spoke = 4;
  positionAlloc->Add (Vector (0.0,0.0,0.0));
  for(uint32_t i=1;i<=nWifi;i++)
  {
    if (i%spoke==1)
    {
      positionAlloc->Add (Vector (d,0.0,0.0));
    }
    else if(i%spoke==2)
    {
      positionAlloc->Add (Vector (0.0,d,0.0));
    }
    else if(i%spoke==3)
    {
      positionAlloc->Add (Vector (-d,0.0,0.0));
    }
    else if(i%spoke==0)
    {
      positionAlloc->Add (Vector (0.0,-d,0.0));
      d++;
    }    
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  // Internet Stack
  InternetStackHelper stack;
  stack.Install (wifiApNode); 
  stack.Install (wifiStaNodes);

  // IP Address Assignment (referred from tutorial [2])
  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer staNodeInterface;
  Ipv4InterfaceContainer apNodeInterface;
  staNodeInterface = address.Assign (staDevices);
  apNodeInterface = address.Assign (apDevice);

  // APP Layer (referred from [5] and [7])
  ApplicationContainer sourceApp, sinkApp;

  uint32_t portNumber = 9;
  for (uint32_t index = 0; index < nWifi; ++index)
    {
      auto ipv4 = wifiApNode.Get (0)->GetObject<Ipv4> ();
      const auto address = ipv4->GetAddress (1, 0).GetLocal ();
      InetSocketAddress sinkSocket (address, portNumber++);
      OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkSocket);
      onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onOffHelper.SetAttribute ("DataRate", DataRateValue (5000000/nWifi));
      onOffHelper.SetAttribute ("PacketSize", UintegerValue (1023)); // 8184 bits = 1023 bytes
      sourceApp.Add (onOffHelper.Install (wifiStaNodes.Get (index)));
      PacketSinkHelper pktsink ("ns3::UdpSocketFactory", sinkSocket);
      sinkApp.Add (pktsink.Install (wifiApNode.Get (0)));
    }

  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (simTime + 1));
  sourceApp.Start (Seconds (1.0));
  sourceApp.Stop (Seconds (simTime + 1));

  // Enabling internetworking routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (simTime+1));
  Simulator::Run ();

  double throughput = 0;
  for (uint32_t index = 0; index < sinkApp.GetN (); ++index)
    {
      uint64_t totalPacketsThrough = DynamicCast<PacketSink> (sinkApp.Get (index))->GetTotalRx ();
      throughput = ((totalPacketsThrough * 8) / (simTime * 1000000.0)); //Mbit/s   
    }

  std::cout << "Number of Stations:\t" << nWifi << std::endl;
  std::cout << "throughput:\t" << throughput << "Mbps" << std::endl;
  dataset.Add(nWifi,throughput);
  Simulator::Destroy ();
}

plot.AddDataset (dataset);
std::ofstream plotFile (plotFilename.c_str());
plot.GenerateOutput (plotFile);
plotFile.close();
}


/*
REFERENCES:
[1] https://pdfs.semanticscholar.org/4a5c/
[2] https://www.nsnam.org/docs/tutorial/html/building-topologies.html
[3] https://www.nsnam.org/doxygen/csma-star_8cc_source.html
[4] https://www.nsnam.org/docs/manual/html/gnuplot.html
[5] https://www.nsnam.org/doxygen/wifi-multi-tos_8cc_source.html
[6] https://www.nsnam.org/doxygen/wifi-spectrum-saturation-example_8cc_source.html
[7] https://www.nsnam.org/doxygen/wifi-hidden-terminal_8cc_source.html?fbclid=IwAR1tKXmtt2kpp0UZ4ZFpbCIX2wZVGosZC62B8mq4T70cNqLK2gFJp0N1KAA
*/
