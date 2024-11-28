#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include <random>

#include "ns3/assert.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/tcp-stream-helper.h"
#include "ns3/tcp-stream-interface.h"
#include "ns3/tcp-stream-controller.h"

#include "matriz.h"
#include "zipf.h"

using namespace ns3;

std::string ns3_dir;

matriz<int> link_delays(10, {
//   0   1   2   3   4   5   6   7   8   9
	-1, 12, -1, -1,  8,  7, -1, -1, -1, -1,// 0

	12, -1, 20, -1, -1, -1, -1, -1, -1, -1,// 1

	-1, 20, -1,  5, -1, -1, 22, -1, -1, -1,// 2

	-1, -1,  5, -1, 10, 13, -1, 21, -1, -1,// 3

	 8, -1, -1, 10, -1, -1, 26, -1, 14,  7,// 4

	 7, -1, -1, 13, -1, -1, -1, -1, -1, -1,// 5

	-1, -1, 22, -1, 26, -1, -1, 21, -1, -1,// 6

	-1, -1, -1, 21, -1, -1, 21, -1, 10, -1,// 7

	-1, -1, -1, -1, 14, -1, -1, 10, -1, 12,// 8

	-1, -1, -1, -1,  7, -1, -1, -1, 12, -1 // 9
	}
);

unsigned int numFogNodes = 9;
uint32_t numberOfServers;
uint32_t numberOfClients;
std::vector <int> qualis {4,3,5,6,2,7,1,0};

double averageArrival = 0.4;
double lamda = 1 / averageArrival;
std::mt19937 rng (0);
std::exponential_distribution<double> poi (lamda);
std::uniform_int_distribution<> dis(0, 9);
std::uniform_real_distribution<double> unif(0, 1);

double sumArrivalTimes=3.6;
double newArrivalTime;

std::ofstream StallsLog;
std::ofstream RebufferLog;
std::ofstream StartTimeLog;
std::ofstream ServerScoreLog;
std::ofstream ClientsLog;

bool IsTopLevelSourceDir (std::string path)
{
	bool haveVersion = false;
	bool haveLicense = false;

	//
	// If there's a file named VERSION and a file named LICENSE in this
	// directory, we assume it's our top level source directory.
	//

	std::list<std::string> files = SystemPath::ReadFiles (path);
	for (std::list<std::string>::const_iterator i = files.begin (); i != files.end (); ++i)
	{
		if (*i == "VERSION")
		{
			haveVersion = true;
		}
		else if (*i == "LICENSE")
		{
			haveLicense = true;
		}
	}

	return haveVersion && haveLicense;
}

std::string GetTopLevelSourceDir (void)
{
	std::string self = SystemPath::FindSelfDirectory ();
	std::list<std::string> elements = SystemPath::Split (self);
	while (!elements.empty ())
	{
		std::string path = SystemPath::Join (elements.begin (), elements.end ());
		if (IsTopLevelSourceDir (path))
		{
			return path + "/";
		}
		elements.pop_back ();
	}
	NS_FATAL_ERROR ("Could not find source directory from self=" << self);
}

void install_mobility(NodeContainer staticNodes)
{
	Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
	allocator->Add (Vector(5, 0, 0));
	allocator->Add (Vector(0, 0, 0));
	allocator->Add (Vector(0, 10, 0));
	allocator->Add (Vector(5, 10, 0));
	allocator->Add (Vector(15, 5, 0));
	allocator->Add (Vector(15, 0, 0));
	allocator->Add (Vector(0, 15, 0));
	allocator->Add (Vector(15, 15, 0));
	allocator->Add (Vector(20, 10, 0));
	allocator->Add (Vector(15, 6, 0));

	MobilityHelper staticNodesHelper;
	staticNodesHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	staticNodesHelper.SetPositionAllocator (allocator);
	staticNodesHelper.Install(staticNodes);
}

uint32_t
uniformDis()
{
  uint32_t value=dis.operator() (rng);
  std::cout << "Ilha:  " << value << std::endl;
  return value;
}

double
poisson()
{
  newArrivalTime=poi.operator() (rng);// generates the next random number in the distribution
  sumArrivalTimes=sumArrivalTimes + newArrivalTime;
  std::cout << "newArrivalTime:  " << newArrivalTime  << "    ,sumArrivalTimes:  " << sumArrivalTimes << std::endl;
  if (sumArrivalTimes<3.6)
  {
    sumArrivalTimes=3.6;
  }
  return sumArrivalTimes;
}

void
InitializeLogFiles (std::string dashLogDirectory, std::string m_algoName,std::string numberOfClients, std::string simulationId,std::string pol)
{
  NS_LOG_UNCOND("Inicializando log");
  std::string SLog = dashLogDirectory + m_algoName + "/" +  numberOfClients  + "/" + pol + "/sim" + simulationId + "_" + "StallLog.csv";
  StallsLog.open (SLog.c_str ());
  StallsLog << "Time_Now;SV1_Stalls;SV1_Stalls_MME;SV2_Stalls;SV2_Stalls_MME;SV3_Stalls;SV3_Stalls_MME;Cloud_Stalls;Cloud_Stalls_MME\n";
  StallsLog.flush ();

  std::string RLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "RebufferLog.csv";
  RebufferLog.open (RLog.c_str ());
  RebufferLog << "Time_Now;SV1_Rebuffer;SV1_Rebuffer_MME;SV2_Rebuffer;SV2_Rebuffer_MME;SV3_Rebuffer;SV3_Rebuffer_MME;Cloud_Rebuffer;Cloud_Rebuffer_MME\n";
  RebufferLog.flush ();

  std::string STLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "PlaybackStartTime.csv";
  StartTimeLog.open (STLog.c_str ());
  StartTimeLog << "SV1_PlaybackStartTime;SV1_PlaybackStartTime_MME;SV2_PlaybackStartTime;SV2_PlaybackStartTime_MME;SV3_PlaybackStartTime;SV3_PlaybackStartTime_MME;Cloud_PlaybackStartTime;Cloud_PlaybackStartTime_MME\n";
  StartTimeLog.flush ();

  std::string SsLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "ServerScores.csv";
  ServerScoreLog.open (SsLog.c_str ());
  ServerScoreLog << "SV1_Score;SV2_Score;SV3_Score;Cloud_Score;\n";
  ServerScoreLog.flush ();

  std::string CLog = dashLogDirectory + m_algoName + "/" +  numberOfClients + "/" + pol + "/sim" + simulationId + "_" + "ClientsBegin.csv";
  ClientsLog.open (CLog.c_str ());
  ClientsLog << "ID;Location;ContentId;ContentQuality;ContentSize;StartTime\n";
  ClientsLog.flush ();
}

int main(int argc, char* argv[])
{
  uint32_t simulationId = 0;
  uint32_t seed = 4242;
  numberOfClients = 1;
  uint64_t segmentDuration = 40000;
  std::string adaptationAlgo = "festive";
  std::string segmentSizeFilePath = "src/Fog4VR/dash/segmentSizesBigBuckVR.txt";
	bool enableAnimation = false;
	short simTime = 200;
  std::string dirTmp("./");
  std::vector< Ipv4Address > fogAddress;
  uint16_t pol=4;

	CommandLine cmd;
	cmd.AddValue ("simulationId", "The simulation's index (for logging purposes)", simulationId);
	cmd.AddValue ("seed", "random seed value.", seed);
	cmd.AddValue ("fogNodes", "Number of fog nodes.", numFogNodes);
	cmd.AddValue ("numberOfClients", "The number of clients", numberOfClients);
	cmd.AddValue ("segmentDuration", "The duration of a video segment in microseconds", segmentDuration);
	cmd.AddValue ("segmentSizeFile", "The relative path (from ns-3.x directory) to the file containing the segment sizes in bytes", segmentSizeFilePath);
	cmd.AddValue ("adaptationAlgo", "The adaptation algorithm that the client uses for the simulation", adaptationAlgo);
	cmd.AddValue("politica", "value to choose the type of politica to be used (0 is AHP , 1 is Greedy, 2 is random and 3 is none. Default is 3)", pol);
	cmd.AddValue ("enableAnimation","Enable generation of animation files", enableAnimation);
	cmd.Parse(argc, argv);

  //Cloud + FogNodes
  numberOfServers = 1 + numFogNodes;

	NS_ASSERT(numberOfServers <= link_delays.getRows());

	ns3::RngSeedManager::SetSeed(seed);
	ns3_dir = GetTopLevelSourceDir();

	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1446));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue (524288));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (524288));

  Ptr<Node> cloud;

	NodeContainer fogNodes;
	fogNodes.Create(numFogNodes);

  NodeContainer serverNodes;
  serverNodes.Add(cloud);
  serverNodes.Add(fogNodes);

  NodeContainer clientNodes;
  clientNodes.Create(numberOfClients);

	install_mobility(serverNodes);

	//------------------------------------------------------------//
	//instalar protocolo ip
	InternetStackHelper internet;
	internet.Install(serverNodes);

	//------------------------------------------------------------//
	//Criar topologia da rede e atribuir endereços
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gb/s")));
	p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));

	Ipv4AddressHelper ipv4h;
	// In point-to-point links we use a /30 subnet which can hold exactly two
	// addresses (remember that net broadcast and null address are not valid)
	ipv4h.SetBase("1.0.0.0", "255.255.255.252");
	for(unsigned int i=0; i<numFogNodes; i++) {
		for(unsigned int j=i+1; j<numFogNodes; j++) {
			if(link_delays[i][j] == -1)
				continue;
			p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(link_delays[i][j])));
			NetDeviceContainer devs = p2ph.Install(serverNodes.Get(i), serverNodes.Get(j));
			ipv4h.Assign(devs);
			//increase network number
			ipv4h.NewNetwork();
		}
	}

	Ptr < Ipv4 > ipv4 = cloud->GetObject < Ipv4 > ();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
	Ipv4Address cloudAddress = iaddr.GetLocal();
	for (uint16_t u = 0; u < numFogNodes; u++)
	{
		Ptr < Node > node = fogNodes.Get(u);
		ipv4 = node->GetObject < Ipv4 > ();
		//Ptr < Address > addr = node->GetObject < Address > ();
		iaddr = ipv4->GetAddress(1, 0);
		Ipv4Address addri = iaddr.GetLocal();
		NS_LOG_UNCOND(addri);
		fogAddress.push_back(addri);
	}

	uint16_t port = 9;

	Controller (numberOfServers, simulationId, dirTmp);
  std::vector<uint16_t> content;
  ApplicationContainer serverApp;
  TcpStreamServerHelper serverHelper (port,simulationId,dirTmp);
  for (uint16_t u = 0; u < serverNodes.GetN(); u++)
  {
    serverHelper.SetAttribute ("RemoteAddress", AddressValue (fogAddress[u]));
    serverHelper.SetAttribute ("ServerId", UintegerValue (u));
    serverApp = serverHelper.Install (serverNodes.Get(u));
    if (u<=1 or u==5 or u==9)
    {
      serverInitialise (u,16000,16000, content, fogAddress[u],0.000085);
    }
    else
    {
      if (u==7)
      {
        serverInitialise (u,64000,64000, content, fogAddress[u],0.00034);
      }
      else
      {
        serverInitialise (u,32000,32000, content, fogAddress[u],0.00017);
      }
    }

    printInformation (u);
  }
  serverApp.Start (Seconds (1.0));

   /* Install TCP/UDP Transmitter on the station */
  InitializeLogFiles (dashLogDirectory, adaptationAlgo, to_string(numberOfClients), to_string(simulationId), to_string(pol));

  ipv4h.SetBase("192.168.0.0", "255.255.255.0");
  PointToPointHelper p2ph_users;
  p2ph_users.SetDeviceAttribute ("DataRate", StringValue ("100Mb/s")); // This must not be more than the maximum throughput in 802.11n
  p2ph_users.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph_users.SetChannelAttribute ("Delay", StringValue ("1ms"));

  TcpStreamClientHelper clientHelper (cloudAddress, port,pol);
  clientHelper.SetAttribute ("SegmentDuration", UintegerValue (segmentDuration));
  clientHelper.SetAttribute ("NumberOfClients", UintegerValue(numberOfClients));
  clientHelper.SetAttribute ("SimulationId", UintegerValue (simulationId));
  //clientHelper.SetAttribute ("RemoteAddress", AddressValue (PAAddress));
  ApplicationContainer clientApps ;//= clientHelper.Install (clients_temp0);
  for (uint i = 0; i < numberOfClients; i++)
  {
    int cont = zipf(0.7,100);
    int quali= zipfQuality(0.7,7);
    quali=qualis[quali];
    //segmentSizeFilePath = "src/Fog4VR/dash/segmentSizesBigBuck"+ToString(quali)+".txt";
    uint32_t value = uniformDis();
    NetDeviceContainer ClientsNet = p2ph_users.Install (networkNodes.Get(value),staContainer.Get (i));
    Ipv4InterfaceContainer wanInterface6 = address6.Assign (ClientsNet);
    address6.NewNetwork();
    std::vector <std::pair <Ptr<Node>, std::string> > clients_temp0;
    clients_temp0.push_back(clients.at(i));
    clientHelper.SetAttribute ("ServerId", UintegerValue (value));
    clientHelper.SetAttribute ("ContentId", UintegerValue (cont));
    clientHelper.SetAttribute ("ContentSize", DoubleValue (sizes[quali]));
    clientHelper.SetAttribute ("SegmentSizeFilePath", StringValue (segmentSizeFilePath));
    //clientHelper.SetAttribute ("RemoteAddress", AddressValue (choiceServer(5,2000)));
    clientApps.Add(clientHelper.Install (clients_temp0,i));
    Ptr<Application> app=clientApps.Get(i);
    double t=poisson();
    app->SetStartTime (Seconds(t));
    LogClient(i,value,cont,quali,sizes[quali],t);
  }

	/* Populate routing table */

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>
  ("dynamic-global-routing.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (5), routingStream);

	AnimationInterface* animator;
	if(enableAnimation) {
		animator = new AnimationInterface ("fog.xml");
		animator->SetMaxPktsPerTraceFile(10000000);
		for (uint32_t j = 0; j < serverNodes.GetN(); ++j) {
			animator->UpdateNodeDescription(serverNodes.Get(j), "Fog " + std::to_string(j));
			animator->UpdateNodeColor(serverNodes.Get(j), 20, 10, 145);
			animator->UpdateNodeSize(serverNodes.Get(j)->GetId(),0.5,0.5);
		}
	}

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();
}
