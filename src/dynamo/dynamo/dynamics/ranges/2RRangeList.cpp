/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <dynamo/dynamics/ranges/2RRangeList.hpp>
#include <dynamo/simulation/particle.hpp>
#include <boost/foreach.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>

namespace dynamo {
  C2RRangeList::C2RRangeList(const magnet::xml::Node& XML, const dynamo::SimData* nSim):
    SimBase_const(nSim, "C2RRangeList")
  { operator<<(XML); }

  bool 
  C2RRangeList::isInRange(const Particle&p1, const Particle&p2) const
  {
    BOOST_FOREACH(const std::tr1::shared_ptr<C2Range>& rPtr, ranges)
      if (rPtr->isInRange(p1,p2))
	return true;
  
    return false;
  }

  void 
  C2RRangeList::operator<<(const magnet::xml::Node& XML)
  {
    if (strcmp(XML.getAttribute("Range"),"RangeList"))
      M_throw() << "Attempting to load a List from a non List";    
  
    try 
      {
	for (magnet::xml::Node node = XML.fastGetNode("RangeListItem"); node.valid(); ++node)
	  ranges.push_back(std::tr1::shared_ptr<C2Range>(C2Range::getClass(node, Sim)));
      }
    catch (boost::bad_lexical_cast &)
      {
	M_throw() << "Failed a lexical cast in C2RRangeList";
      }
  }

  void 
  C2RRangeList::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Range") << "RangeList";

    BOOST_FOREACH(const std::tr1::shared_ptr<C2Range>& rPtr, ranges)
      XML << magnet::xml::tag("RangeListItem") << rPtr << magnet::xml::endtag("RangeListItem");
  }
}
