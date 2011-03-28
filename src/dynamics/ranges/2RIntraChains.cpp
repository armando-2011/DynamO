/*  DYNAMO:- Event driven molecular dynamics simulator 
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

#include "2RIntraChains.hpp"
#include "../../extcode/xmlParser.h"
#include "../../simulation/particle.hpp"
#include <boost/lexical_cast.hpp>
#include <magnet/xmlwriter.hpp>


C2RIntraChains::C2RIntraChains(unsigned long r1, unsigned long r2, unsigned long r3):
  range1(r1),range2(r2),interval(r3) 
{
  if ((r2-r1 + 1) % r3)
    M_throw() << "Range of C2RIntraChains does not split evenly into interval";
}

C2RIntraChains::C2RIntraChains(const XMLNode& XML, const DYNAMO::SimData*):
  range1(0),range2(0), interval(0)
{ 
  if (strcmp(XML.getAttribute("Range"),"IntraChains"))
    M_throw() << "Attempting to load a chains from a non chains";
  
  range1 = boost::lexical_cast<unsigned long>(XML.getAttribute("Start"));
  range2 = boost::lexical_cast<unsigned long>(XML.getAttribute("End"));
  interval = boost::lexical_cast<unsigned long>(XML.getAttribute("Interval"));

  if ((range2-range1 + 1) % interval)
    M_throw() << "Range of C2RIntraChains does not split evenly into interval";
}

bool 
C2RIntraChains::isInRange(const Particle&p1, const Particle&p2) const
{
  //A version with no ? : operators at the expense of one more <=
  //operator, seems fastest
  return     
    //Leave this till last as its expensive. Actually put it first as
    //if you're in a system of chains the other statements are always
    //true
    (((p1.getID() - range1) / interval)
	== ((p2.getID() - range1) / interval)) 
    //Test its within the range
    && (p2.getID() >= range1) && (p2.getID() <= range2) 
    && (p1.getID() >= range1) && (p1.getID() <= range2);
}

void 
C2RIntraChains::operator<<(const XMLNode&)
{
  M_throw() << "Due to problems with CRAll C2RIntraChains::operator<< cannot work for this class";
}

void 
C2RIntraChains::outputXML(xml::XmlStream& XML) const
{
  XML << xml::attr("Range") << "IntraChains" 
      << xml::attr("Start")
      << range1
      << xml::attr("End")
      << range2
      << xml::attr("Interval")
      << interval;
}

