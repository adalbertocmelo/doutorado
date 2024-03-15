/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "tcp-multi-stream-helper.h"
#include "ns3/tcp-multi-stream-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/names.h"

namespace ns3 {

TcpMultiStreamClientHelper::TcpMultiStreamClientHelper (Address address, uint16_t port, uint16_t pol)
{ //NS_LOG_UNCOND("clienthelper construtor 77");
  m_factory.SetTypeId (TcpMultiStreamClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("PolId", UintegerValue (pol));
}

TcpMultiStreamClientHelper::TcpMultiStreamClientHelper (Ipv4Address address, uint16_t port, uint16_t pol)
{ //NS_LOG_UNCOND("clienthelper construtor 84");
  m_factory.SetTypeId (TcpMultiStreamClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("PolId", UintegerValue (pol));
}

TcpMultiStreamClientHelper::TcpMultiStreamClientHelper (Ipv6Address address, uint16_t port, uint16_t pol)
{ //NS_LOG_UNCOND("clienthelper construtor 91");
  m_factory.SetTypeId (TcpMultiStreamClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("PolId", UintegerValue (pol));
}

void
TcpMultiStreamClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{ //NS_LOG_UNCOND("clienthelper setAttribute 99");
  m_factory.Set (name, value);
}

ApplicationContainer
TcpMultiStreamClientHelper::Install (std::vector <std::pair <Ptr<Node>, std::string> > clients) const
{ //NS_LOG_UNCOND("clienthelper Install 105");
  ApplicationContainer apps;
  for (uint i = 0; i < clients.size (); i++)
    {
      apps.Add (InstallPriv (clients.at (i).first, clients.at (i).second, i));
    }

  return apps;
}

ApplicationContainer
TcpMultiStreamClientHelper::Install (std::vector <std::pair <Ptr<Node>, std::string> > clients, uint i) const
{ //NS_LOG_UNCOND("clienthelper Install 105");
  ApplicationContainer apps;
  apps.Add (InstallPriv (clients.at (0).first, clients.at (0).second, i));
  return apps;
}


void
TcpMultiStreamClientHelper::Handover(ApplicationContainer clientApps, Ptr<Node> node, Address ip)
{ //NS_LOG_UNCOND("clientHelper Handover 118");
  Ptr<Application> app = node->GetApplication(0);
  app->GetObject<TcpMultiStreamClient> ()->SetHandover(ip);
}

double
TcpMultiStreamClientHelper::GetTotalBufferUnderrunTime(ApplicationContainer clientApps, Ptr<Node> node)
{
  double t;
  Ptr<Application> app = node->GetApplication(0);
  t=app->GetObject<TcpMultiStreamClient> ()->GetBufferUnderrunTotalTime();
  return t;
}

uint64_t
TcpMultiStreamClientHelper::GetNumbersOfBufferUnderrun(ApplicationContainer clientApps, Ptr<Node> node)
{
  uint64_t i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetBufferUnderrunCount();
  return i;
}

uint64_t
TcpMultiStreamClientHelper::GetRepIndex(ApplicationContainer clientApps, Ptr<Node> node)
{
  uint64_t i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetRepIndex();
  switch(i)
  {
    case 0:
      i=400;
      break;
    case 1:
      i=650;
      break;
    case 2:
      i=1000;
      break;
    case 3:
      i=1500;
      break;
    case 4:
      i=2250;
      break;
    case 5:
      i=3400;
      break;
    case 6:
      i=4700;
      break;
    case 7:
      i=6000;
      break;
  }
  return i;
}

double
TcpMultiStreamClientHelper::GetThroughput(ApplicationContainer clientApps, Ptr<Node> node)
{
  double i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetThroughput();
  return i;
}

double
TcpMultiStreamClientHelper::GetPlaybakStartTime(ApplicationContainer clientApps, Ptr<Node> node)
{
  double i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetPlaybackStart();
  return i;
}

std::string
TcpMultiStreamClientHelper::GetServerAddress(ApplicationContainer clientApps, Ptr<Node> node)
{
  std::string i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetServerAddress();
  return i;
}

std::string
TcpMultiStreamClientHelper::GetNewServerAddress(ApplicationContainer clientApps, Ptr<Node> node)
{
  std::string i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->GetNewServerAddress();
  return i;
}

bool
TcpMultiStreamClientHelper::GetHandover(ApplicationContainer clientApps, Ptr<Node> node)
{
  bool i;
  Ptr<Application> app = node->GetApplication(0);
  i=app->GetObject<TcpMultiStreamClient> ()->checkHandover();
  return i;
}

Ptr<Application>
TcpMultiStreamClientHelper::InstallPriv (Ptr<Node> node, std::string algo, uint16_t clientId) const
{
  Ptr<Application> app = m_factory.Create<TcpMultiStreamClient> ();
  app->GetObject<TcpMultiStreamClient> ()->SetAttribute ("ClientId", UintegerValue (clientId));
  app->GetObject<TcpMultiStreamClient> ()->Initialise (algo, clientId);
  node->AddApplication (app);
  //app->GetObject<TcpMultiStreamClient> ()->SetRemote (Ipv4Address ip, uint16_t port);
  return app;
}

uint32_t
TcpMultiStreamClientHelper::checkApps(NodeContainer staContainer)
{ //NS_LOG_UNCOND("checkApps");
  uint32_t closedApps=0;
  bool c;
  uint32_t nNodes = staContainer.GetN ();
  for (uint32_t i = 0; i < nNodes; ++i)
  {
    Ptr<Node> p = staContainer.Get (i);
    Ptr<Application> app = p->GetApplication(0);
    c=app->GetObject<TcpMultiStreamClient> ()->check ();
    if (c==true)
    {
      closedApps++;
    }
  }
  return closedApps;
}

} // namespace ns3
