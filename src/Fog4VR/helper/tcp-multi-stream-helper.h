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
#ifndef TCP_MULTI_STREAM_HELPER_H
#define TCP_MULTI_STREAM_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

/**
 * \ingroup TcpStream
 * \brief Create an application which sends a UDP packet and waits for an echo of this packet
 */
class TcpMultiStreamClientHelper
{
public:
  /**
   * Create TcpMultiStreamClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IP address of the remote tcp stream server
   * \param port The port number of the remote tcp stream server
   */
  TcpMultiStreamClientHelper (Address ip, uint16_t port, uint16_t polId);
  /**
   * Create TcpMultiStreamClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IPv4 address of the remote tcp stream server
   * \param port The port number of the remote tcp stream server
   */
  TcpMultiStreamClientHelper (Ipv4Address ip, uint16_t port, uint16_t polId);
  /**
   * Create TcpMultiStreamClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IPv6 address of the remote tcp stream server
   * \param port The port number of the remote tcp stream server
   */
  TcpMultiStreamClientHelper (Ipv6Address ip, uint16_t port, uint16_t polId);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * \param clients the nodes with the name of the adaptation algorithm to be used
   *
   * Create one tcp stream client application on each of the input nodes and
   * instantiate an adaptation algorithm on each of the tcp stream client according
   * to the given string.
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (std::vector <std::pair <Ptr<Node>, std::string> > clients) const;

  ApplicationContainer Install (std::vector <std::pair <Ptr<Node>, std::string> > clients, uint i) const;

  uint32_t checkApps(NodeContainer staContainer);

  void Handover(ApplicationContainer clientApps, Ptr<Node> node, Address ip);

  double GetTotalBufferUnderrunTime(ApplicationContainer clientApps, Ptr<Node> node);

  uint64_t GetNumbersOfBufferUnderrun(ApplicationContainer clientApps, Ptr<Node> node);

  double GetThroughput(ApplicationContainer clientApps, Ptr<Node> node);

  double GetPlaybakStartTime(ApplicationContainer clientApps, Ptr<Node> node);

  std::string GetServerAddress(ApplicationContainer clientApps, Ptr<Node> node);

  std::string GetNewServerAddress(ApplicationContainer clientApps, Ptr<Node> node);

  bool GetHandover(ApplicationContainer clientApps, Ptr<Node> node);

  uint64_t GetRepIndex(ApplicationContainer clientApps, Ptr<Node> node);

private:
  /**
   * Install an ns3::TcpMultiStreamClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an TcpMultiStreamClient will be installed.
   * \param algo A string containing the name of the adaptation algorithm to be used on this client
   * \param clientId distinguish this client object from other parallel running clients, for logging purposes
   * \param simulationId distinguish this simulation from other subsequently started simulations, for logging purposes
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node, std::string algo, uint16_t clientId) const;
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* TCP_MULTI_STREAM_HELPER_H */
