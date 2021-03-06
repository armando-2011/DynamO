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

#include <dynamo/schedulers/complexentries/nblistentry.hpp>
#include <dynamo/schedulers/scheduler.hpp>
#include <dynamo/dynamics/globals/neighbourList.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <magnet/xmlwriter.hpp>
#include <magnet/xmlreader.hpp>

namespace dynamo {
  SCENBList::SCENBList(const magnet::xml::Node& XML, dynamo::SimData* const nSim):
    SCEntry(nSim, "ComplexNBlistEntry"),
    nblistID(std::numeric_limits<size_t>::max())
  {
    operator<<(XML);
  }

  void 
  SCENBList::operator<<(const magnet::xml::Node& XML)
  {
    range = std::tr1::shared_ptr<CRange>(CRange::getClass(XML, Sim));
  
    try 
      {
	if (strcmp(XML.getAttribute("Type"),"NeighbourList"))
	  M_throw() << "Attempting to load NeighbourList from "
		    << XML.getAttribute("Type") << " entry";
  
	name = XML.getAttribute("NBListName");
      }
    catch (boost::bad_lexical_cast &)
      {
	M_throw() << "Failed a lexical cast in SCENBList";
      }
  }

  void 
  SCENBList::initialise()
  {
    try {
      nblistID = Sim->dynamics.getGlobal(name)->getID();
    }
    catch (std::exception& cep)
      {
	M_throw() << "Failed to find the global named " 
		  << name << " for the SCENBList entry."
		  << "\n" << cep.what();
      }

    if (!std::tr1::dynamic_pointer_cast<GNeighbourList>(Sim->dynamics.getGlobals()[nblistID]))
      M_throw() << "Global named " << name << " is not a GNeighbourList";
  
    GNeighbourList& nblist = static_cast<GNeighbourList&>(*Sim->dynamics.getGlobals()[nblistID]);
    nblist.markAsUsedInScheduler();

    nblist.ConnectSigNewNeighbourNotify<Scheduler>(&Scheduler::addInteractionEvent, Sim->ptrScheduler.get());
    nblist.ConnectSigNewLocalNotify<Scheduler>(&Scheduler::addLocalEvent, Sim->ptrScheduler.get());
  }

  void 
  SCENBList::getParticleNeighbourhood(const Particle& part, 
				       const GNeighbourList::nbHoodFunc& func) const
  {
#ifdef DYNAMO_DEBUG
    if (!isApplicable(part))
      M_throw() << "This complexNBlist entry ("
		<< name << ") is not valid for this particle (" 
		<< part.getID() << ") yet it is being used anyway!";
#endif

    static_cast<const GNeighbourList&>(*Sim->dynamics.getGlobals()[nblistID])
      .getParticleNeighbourhood(part, func);
  }

  void 
  SCENBList::getParticleNeighbourhood(const Vector& vec, 
				       const GNeighbourList::nbHoodFunc2& func) const
  {
    static_cast<const GNeighbourList&>(*Sim->dynamics.getGlobals()[nblistID])
      .getParticleNeighbourhood(vec, func);
  }

  void 
  SCENBList::getLocalNeighbourhood(const Particle& part, 
				    const GNeighbourList::nbHoodFunc& func) const
  {
#ifdef DYNAMO_DEBUG
    if (!isApplicable(part))
      M_throw() << "This complexNBlist entry ("
		<< name << ") is not valid for this particle (" 
		<< part.getID() << ") yet it is being used anyway!";
#endif

    static_cast<const GNeighbourList&>(*Sim->dynamics.getGlobals()[nblistID])
      .getLocalNeighbourhood(part, func);
  }


  void 
  SCENBList::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Type") << "NeighbourList"
	<< magnet::xml::attr("NBListName")
	<< name
	<< range
      ;
  }
}
