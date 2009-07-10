/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2008  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "socells.hpp"
#include "globEvent.hpp"
#include "../NparticleEventData.hpp"
#include "../../extcode/xmlParser.h"
#include "../../extcode/xmlwriter.hpp"
#include "../liouvillean/liouvillean.hpp"
#include "../units/units.hpp"
#include "../ranges/1RAll.hpp"
#include "../../schedulers/scheduler.hpp"
#include "../locals/local.hpp"
#include "../BC/LEBC.hpp"
#include <boost/static_assert.hpp>
#include <boost/math/special_functions/pow.hpp>


CGSOCells::CGSOCells(DYNAMO::SimData* nSim, const std::string& name):
  CGlobal(nSim, "SingleOccupancyCells"),
  cellCount(0),
  cellDimension(1,1,1),
  cuberootN(0)
{
  globName = name;
  I_cout() << "Single occupancy cells loaded";
}

CGSOCells::CGSOCells(const XMLNode &XML, DYNAMO::SimData* ptrSim):
  CGlobal(ptrSim, "SingleOccupancyCells"),
  cellCount(0),
  cellDimension(1,1,1),
  cuberootN(0)
{
  operator<<(XML);

  I_cout() << "Single occupancy cells loaded";
}

void 
CGSOCells::operator<<(const XMLNode& XML)
{
  try {
    globName = XML.getAttribute("Name");	
  }
  catch(...)
    {
      D_throw() << "Error loading CGSOCells";
    }
}

CGlobEvent 
CGSOCells::getEvent(const CParticle& part) const
{
#ifdef ISSS_DEBUG
  if (!Sim->Dynamics.Liouvillean().isUpToDate(part))
    D_throw() << "Particle is not up to date";
#endif

  //This 
  //Sim->Dynamics.Liouvillean().updateParticle(part);
  //is not required as we compensate for the delay using 
  //Sim->Dynamics.Liouvillean().getParticleDelay(part)

  Vector CellOrigin;
  size_t ID(part.getID());

  for (size_t iDim(0); iDim < NDIM; ++iDim)
    {
      CellOrigin[iDim] = (ID % cuberootN) * cellDimension[iDim] - 0.5*Sim->aspectRatio[iDim];
      ID /= cuberootN;
    }

  return CGlobEvent(part,
		    Sim->Dynamics.Liouvillean().
		    getSquareCellCollision2
		    (part, CellOrigin,
		     cellDimension)
		    -Sim->Dynamics.Liouvillean().getParticleDelay(part),
		    CELL, *this);
}

void
CGSOCells::runEvent(const CParticle& part) const
{
  Sim->Dynamics.Liouvillean().updateParticle(part);

  Vector CellOrigin;
  size_t ID(part.getID());

  for (size_t iDim(0); iDim < NDIM; ++iDim)
    {
      CellOrigin[iDim] = (ID % cuberootN) * cellDimension[iDim] - 0.5*Sim->aspectRatio[iDim];
      ID /= cuberootN;
    }
  
  //Determine the cell transition direction, its saved
  size_t cellDirection(Sim->Dynamics.Liouvillean().
		       getSquareCellCollision3
		       (part, CellOrigin, 
			cellDimension));

  CGlobEvent iEvent(getEvent(part));

#ifdef DYNAMO_DEBUG 
  if (isnan(iEvent.getdt()))
    D_throw() << "A NAN Interaction collision time has been found"
	      << iEvent.stringData(Sim);
  
  if (iEvent.getdt() == HUGE_VAL)
    D_throw() << "An infinite Interaction (not marked as NONE) collision time has been found\n"
	      << iEvent.stringData(Sim);
#endif

  Sim->dSysTime += iEvent.getdt();
    
  Sim->ptrScheduler->stream(iEvent.getdt());
  
  Sim->Dynamics.stream(iEvent.getdt());

  Vector vNorm(0,0,0);

  Vector pos(part.getPosition()), vel(part.getVelocity());

  Sim->Dynamics.BCs().setPBC(pos, vel);

  vNorm[cellDirection] = (vel[cellDirection] > 0) ? -1 : +1; 
    
  //Run the collision and catch the data
  CNParticleData EDat(Sim->Dynamics.Liouvillean().runWallCollision
		      (part, vNorm, 1.0));

  Sim->signalParticleUpdate(EDat);

  //Now we're past the event update the scheduler and plugins
  Sim->ptrScheduler->fullUpdate(part);
  
  BOOST_FOREACH(smrtPlugPtr<COutputPlugin> & Ptr, Sim->outputPlugins)
    Ptr->eventUpdate(iEvent, EDat);

}

void 
CGSOCells::initialise(size_t nID)
{
  ID=nID;
  
  cuberootN = (unsigned long)(std::pow(Sim->lN, 1.0/3.0) + 0.5);
  
  if (boost::math::pow<3>(cuberootN) != Sim->lN)
    D_throw() << "Cannot use single occupancy cells without a integer cube root of N"
	      << "\nN = " << Sim->lN
	      << "\nN^(1/3) = " << cuberootN;

  for (size_t iDim(0); iDim < NDIM; ++iDim)
    cellDimension[iDim] = Sim->aspectRatio[iDim] / cuberootN;
}

void
CGSOCells::outputXML(xmlw::XmlStream& XML) const
{
  XML << xmlw::attr("Type") << "SOCells"
      << xmlw::attr("Name") << globName;
}