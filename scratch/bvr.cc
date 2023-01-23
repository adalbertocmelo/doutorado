#include <cstdlib>
#include <list>
#include <string>
#include <vector>

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

#include "matriz.h"

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

unsigned int numFogNodes=10;

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

int main(int argc, char* argv[])
{
	uint32_t seed = 4242;
	bool enableAnimation = false;
	short simTime = 200;

	CommandLine cmd;
	cmd.AddValue("seed", "random seed value.", seed);
	cmd.AddValue("enableAnimation","Enable generation of animation files", enableAnimation);
	//cmd.AddValue("fogNodes", "Number of fog nodes.", numFogNodes);
	cmd.Parse(argc, argv);

	NS_ASSERT(numFogNodes <= link_delays.getRows());

	ns3::RngSeedManager::SetSeed(seed);
	ns3_dir = GetTopLevelSourceDir();

	NodeContainer FogContainer;
	FogContainer.Create(numFogNodes);

	install_mobility(FogContainer);

	//------------------------------------------------------------//
	//instalar protocolo ip
	InternetStackHelper internet;
	internet.Install(FogContainer);

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
			NetDeviceContainer devs = p2ph.Install(FogContainer.Get(i), FogContainer.Get(j));
			ipv4h.Assign(devs);
			//increase network number
			ipv4h.NewNetwork();
		}
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
		for (uint32_t j = 0; j < FogContainer.GetN(); ++j) {
			animator->UpdateNodeDescription(FogContainer.Get(j), "Fog " + std::to_string(j));
			animator->UpdateNodeColor(FogContainer.Get(j), 20, 10, 145);
			animator->UpdateNodeSize(FogContainer.Get(j)->GetId(),0.5,0.5);
		}
	}

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();
}
