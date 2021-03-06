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

#include <dynamo/dynamics/systems/sysTicker.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/NparticleEventData.hpp>
#include <dynamo/dynamics/liouvillean/liouvillean.hpp>
#include <dynamo/outputplugins/tickerproperty/ticker.hpp>
#include <dynamo/dynamics/units/units.hpp>
#include <dynamo/schedulers/scheduler.hpp>

#ifdef DYNAMO_DEBUG 
#include <boost/math/special_functions/fpclassify.hpp>
#endif

namespace dynamo {
  CSTicker::CSTicker(dynamo::SimData* nSim, double nPeriod, std::string nName):
    System(nSim)
  {
    if (nPeriod <= 0.0)
      nPeriod = Sim->dynamics.units().unitTime();

    dt = nPeriod;
    period = nPeriod;

    sysName = nName;

    dout << "System ticker set for a peroid of " 
	 << nPeriod / Sim->dynamics.units().unitTime() << std::endl;
  }

  void
  CSTicker::runEvent() const
  {
    double locdt = dt;
  
#ifdef DYNAMO_DEBUG 
    if (boost::math::isnan(dt))
      M_throw() << "A NAN system event time has been found";
#endif
    

    Sim->dSysTime += locdt;

    Sim->ptrScheduler->stream(locdt);
  
    //dynamics must be updated first
    Sim->dynamics.stream(locdt);
  
    dt += period;
  
    locdt += Sim->freestreamAcc;
    Sim->freestreamAcc = 0;

    //This is done here as most ticker properties require it
    Sim->dynamics.getLiouvillean().updateAllParticles();

    BOOST_FOREACH(std::tr1::shared_ptr<OutputPlugin>& Ptr, Sim->outputPlugins)
      {
	std::tr1::shared_ptr<OPTicker> ptr = std::tr1::dynamic_pointer_cast<OPTicker>(Ptr);
	if (ptr) ptr->ticker();
      }

    BOOST_FOREACH(std::tr1::shared_ptr<OutputPlugin>& Ptr, Sim->outputPlugins)
      Ptr->eventUpdate(*this, NEventData(), locdt);
  }

  void 
  CSTicker::initialise(size_t nID)
  { ID = nID; }

  void 
  CSTicker::setdt(double ndt)
  { 
    dt = ndt * Sim->dynamics.units().unitTime(); 
  }

  void 
  CSTicker::increasedt(double ndt)
  { 
    dt += ndt * Sim->dynamics.units().unitTime(); 
  }

  void 
  CSTicker::setTickerPeriod(const double& nP)
  { 
    dout << "Setting system ticker period to " 
	 << nP / Sim->dynamics.units().unitTime() << std::endl;

    period = nP; 

    dt = nP;

    if ((Sim->status >= INITIALISED) && Sim->endEventCount)
      Sim->ptrScheduler->rebuildSystemEvents();
  }
}
