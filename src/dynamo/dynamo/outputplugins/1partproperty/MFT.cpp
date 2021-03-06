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

#include <dynamo/outputplugins/1partproperty/MFT.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/dynamics.hpp>
#include <dynamo/dynamics/species/species.hpp>
#include <dynamo/dynamics/1particleEventData.hpp>
#include <dynamo/dynamics/units/units.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>
#include <boost/foreach.hpp>

namespace dynamo {
  OPMFT::OPMFT(const dynamo::SimData* tmp, const magnet::xml::Node& XML):
    OP1PP(tmp,"MeanFreeLength", 250),
    collisionHistoryLength(10),
    binwidth(0.01)
  {
    operator<<(XML);
  }

  void 
  OPMFT::operator<<(const magnet::xml::Node& XML)
  {
    try 
      {
	if (XML.hasAttribute("binwidth"))
	  binwidth = XML.getAttribute("binwidth").as<double>();

	if (XML.hasAttribute("length"))
	  collisionHistoryLength = XML.getAttribute("length").as<size_t>();
      }
    catch (boost::bad_lexical_cast&)
      {
	M_throw() << "Failed a lexical cast in OPMFL";
      }
  }

  void
  OPMFT::initialise()
  {
    lastTime.resize(Sim->N, 
		    boost::circular_buffer<double>(collisionHistoryLength, 0.0));
  
    std::vector<C1DHistogram> vecTemp;
  
    vecTemp.resize(collisionHistoryLength, 
		   C1DHistogram(Sim->dynamics.units().unitTime() * binwidth));
  
    data.resize(Sim->dynamics.getSpecies().size(), vecTemp);
  }

  void 
  OPMFT::A1ParticleChange(const ParticleEventData& PDat)
  {
    //We ignore stuff that hasn't had an event yet

    for (size_t collN = 0; collN < collisionHistoryLength; ++collN)
      if (lastTime[PDat.getParticle().getID()][collN] != 0.0)
	{
	  data[PDat.getSpecies().getID()][collN]
	    .addVal(Sim->dSysTime 
		    - lastTime[PDat.getParticle().getID()][collN]);
	}
  
    lastTime[PDat.getParticle().getID()].push_front(Sim->dSysTime);
  }

  void
  OPMFT::output(magnet::xml::XmlStream &XML)
  {
    XML << magnet::xml::tag("MFT");
  
    for (size_t id = 0; id < data.size(); ++id)
      {
	XML << magnet::xml::tag("Species")
	    << magnet::xml::attr("Name")
	    << Sim->dynamics.getSpecies()[id]->getName();
      
	for (size_t collN = 0; collN < collisionHistoryLength; ++collN)
	  {
	    XML << magnet::xml::tag("Collisions")
		<< magnet::xml::attr("val") << collN + 1;
	  
	    data[id][collN].outputHistogram
	      (XML, 1.0 / Sim->dynamics.units().unitTime());
	  
	    XML << magnet::xml::endtag("Collisions");
	  }
	
	XML << magnet::xml::endtag("Species");
      }
  
    XML << magnet::xml::endtag("MFT");
  }
}
