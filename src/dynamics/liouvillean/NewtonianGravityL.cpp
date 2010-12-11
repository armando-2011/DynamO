/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "NewtonianGravityL.hpp"
#include "../../extcode/xmlwriter.hpp"
#include "../interactions/intEvent.hpp"
#include "../2particleEventData.hpp"
#include "../NparticleEventData.hpp"
#include "../dynamics.hpp"
#include "../BC/BC.hpp"
#include "../../base/is_simdata.hpp"
#include "../species/species.hpp"
#include "../../schedulers/sorters/datastruct.hpp"
#include "shapes/frenkelroot.hpp"
#include "shapes/oscillatingplate.hpp"
#include <boost/math/special_functions/fpclassify.hpp>
#include <magnet/math/cubic.hpp>

LNewtonianGravity::LNewtonianGravity(DYNAMO::SimData* tmp, const XMLNode& XML):
  LNewtonian(tmp),
  Gravity(-1),
  GravityDim(1)
{
  if (strcmp(XML.getAttribute("Type"),"NewtonianGravity"))
    M_throw() << "Attempting to load NewtonianGravity from "
	      << XML.getAttribute("Type")
	      << " entry";
  try 
    {
      if (XML.isAttributeSet("Gravity"))
	Gravity = boost::lexical_cast<double>(XML.getAttribute("Gravity"));      
      
      if (XML.isAttributeSet("GravityDimension"))
	GravityDim = boost::lexical_cast<double>(XML.getAttribute("GravityDimension"));      
    }
  catch (boost::bad_lexical_cast &)
    {
      M_throw() << "Failed a lexical cast in LNewtonianGravity";
    }
  
  Gravity *= Sim->dynamics.units().unitAcceleration();
}

LNewtonianGravity::LNewtonianGravity(DYNAMO::SimData* tmp, double gravity, 
				     size_t gravityDim):
  LNewtonian(tmp), Gravity(gravity), GravityDim(gravityDim) {}

void
LNewtonianGravity::streamParticle(Particle &particle, const double &dt) const
{
  particle.getPosition() += dt * particle.getVelocity();

  bool isDynamic = particle.testState(Particle::DYNAMIC);
  particle.getPosition()[GravityDim] += 0.5 * dt * dt * Gravity * isDynamic;
  particle.getVelocity()[GravityDim] += dt * Gravity * isDynamic;
}

bool 
LNewtonianGravity::SphereSphereInRoot(CPDData& dat, const double& d2, 
				      bool p1Dynamic, bool p2Dynamic) const
{
  if (p1Dynamic == p2Dynamic)
    return LNewtonian::SphereSphereInRoot(dat,d2,p1Dynamic,p2Dynamic);

  Vector g(0,0,0);
  g[GravityDim] = (p1Dynamic) ? Gravity : -Gravity;

  //Generate the coefficients of the quartic
  const double coeffs[5] = {0.25 * Gravity * Gravity,
			    g | dat.vij,
			    dat.v2 + (g | dat.rij),
			    2 * dat.rvdot, 
			    dat.r2 - d2};

  //This value is used to make the derivative of the quartic have a
  //unit coefficient for the t^3 term
  const double cubicnorm = 0.25 / coeffs[0];
  
  //We calculate the roots of the cubic
  double roots[3];
  size_t rootCount = magnet::math::cubicSolve(coeffs[1] * cubicnorm * 3, 
					      coeffs[2] * cubicnorm * 2, 
					      coeffs[3] * cubicnorm, 
					      roots[0], roots[1], roots[2]);
  
  //Sort the roots in ascending order
  std::sort(roots, roots + rootCount);

  double tm = ((rootCount > 1) && (roots[0] < 0)) ? roots[2] : roots[0];

  //Only accept positive minimums (otherwise collision was in the past)
  if (tm < 0) return false;

  //Check an overlap actually occurs at the minimum
  if ((((coeffs[0] * tm + coeffs[1]) * tm + coeffs[2]) * tm + coeffs[3]) * tm + coeffs[4] > 0)
    return false;

  //Now bisect the root

  double t1 = 0, t2 = tm;
  for(size_t i = 0; i < 500; ++i)
    {
      tm = 0.5 * (t1 + t2);

      double f = (((coeffs[0] * tm + coeffs[1]) * tm + coeffs[2]) * tm + coeffs[3]) * tm + coeffs[4];
      if(fabs(f)< 1e-16 && f > 0.0)
	break;
      if(f < 0.0)
	t2 = tm;
      else
	t1 = tm;
    }

  dat.dt = tm;
  return true;
}
  
bool 
LNewtonianGravity::SphereSphereOutRoot(CPDData& dat, const double& d2, bool p1Dynamic, bool p2Dynamic) const
{
  if (p1Dynamic == p2Dynamic)
    return LNewtonian::SphereSphereOutRoot(dat,d2,p1Dynamic,p2Dynamic);

  M_throw() << "Unsupported";
}


double 
LNewtonianGravity::getWallCollision(const Particle &part, 
				    const Vector  &wallLoc, 
				    const Vector  &wallNorm) const
{
  Vector  rij = part.getPosition(),
    vel = part.getVelocity();

  Sim->dynamics.BCs().applyBC(rij, vel);

  double adot = wallNorm[GravityDim] * Gravity * part.testState(Particle::DYNAMIC);
  double vdot = vel | wallNorm;
  double rdot = (rij - wallLoc) | wallNorm;

  double arg = vdot * vdot - 2 * rdot * adot;
  
  if (arg > 0)
    {
      double t = -(vdot + ((vdot<0) ? -1: 1) * std::sqrt(arg));
      double x1 = t / adot;
      double x2 = 2 * rdot / t;

      if (adot > 0)
	//The particle is arcing under the plate
	return (x1 < x2) ? x1 : x2 ;
      else
	//The particle is arcing over the plate
	return (x1 < x2) ? x2 : x1;
    }

  return HUGE_VAL;
}

double
LNewtonianGravity::getSquareCellCollision2(const Particle& part, 
					   const Vector & origin, 
					   const Vector & width) const
{
  Vector rpos(part.getPosition() - origin);
  Vector vel(part.getVelocity());
  Sim->dynamics.BCs().applyBC(rpos, vel);
  
#ifdef DYNAMO_DEBUG
  for (size_t iDim = 0; iDim < NDIM; ++iDim)
    if ((vel[iDim] == 0) && (std::signbit(vel[iDim])))
      M_throw() << "You have negative zero velocities, dont use them."
		<< "\nPlease think of the neighbour lists.";
#endif 

  double retVal = HUGE_VAL;

  for (size_t iDim = 0; iDim < NDIM; ++iDim)
    if ((iDim == GravityDim) && part.testState(Particle::DYNAMIC))
      {
	double adot = Gravity;
	double vdot = vel[GravityDim];

	//First check the "upper" boundary that may have no roots
	double rdot = (Gravity < 0) ? rpos[iDim]-width[iDim] : rpos[iDim];
	double arg = vdot * vdot - 2 * rdot * adot;
	double upperRoot1(HUGE_VAL), upperRoot2(HUGE_VAL);
	
	if (arg >= 0)
	  {
	    double t = -(vdot + ((vdot<0) ? -1: 1) * std::sqrt(arg));
	    upperRoot1 = t / adot;
	    upperRoot1 = 2 * rdot / t;
	    if (upperRoot2 < upperRoot1) std::swap(upperRoot2, upperRoot1);
	  }

	
	//Now the lower boundary which always has roots
	rdot = (Gravity < 0) ? rpos[iDim] : rpos[iDim] - width[iDim];
	arg = vdot * vdot - 2 * rdot * adot;
	double lowerRoot1(HUGE_VAL), lowerRoot2(HUGE_VAL);
	if (arg >= 0)
	  {
	    double t = -(vdot + ((vdot<0) ? -1: 1) * std::sqrt(arg));
	    lowerRoot1 = t / adot;
	    lowerRoot2 = 2 * rdot / t;
	    if (lowerRoot2 < lowerRoot1) std::swap(lowerRoot2, lowerRoot1);
	  }

	double root = HUGE_VAL;
	//Now, if the velocity is "up", and the upper roots exist,
	//then pick the shortest one
	if (!((Gravity < 0) - (vel[GravityDim] > 0))
	    && (upperRoot1 != HUGE_VAL))
	  root = upperRoot1;

	//Otherwise its usually the latest lowerRoot
	if (root == HUGE_VAL)
	  root = lowerRoot2;

	if (root < retVal)
	  retVal = root;
      }
    else
      {
	double tmpdt((vel[iDim] < 0)
		   ? -rpos[iDim]/vel[iDim] 
		   : (width[iDim]-rpos[iDim]) / vel[iDim]);
	
	if (tmpdt < retVal)
	  retVal = tmpdt;
      }
  
  return retVal;
}

int
LNewtonianGravity::getSquareCellCollision3(const Particle& part, 
					   const Vector & origin, 
					   const Vector & width) const
{
  Vector  rpos(part.getPosition() - origin);
  Vector  vel(part.getVelocity());

  Sim->dynamics.BCs().applyBC(rpos, vel);

  int retVal(0);
  double time(HUGE_VAL);
  
#ifdef DYNAMO_DEBUG
  for (size_t iDim = 0; iDim < NDIM; ++iDim)
    if ((vel[iDim] == 0) && (std::signbit(vel[iDim])))
      M_throw() << "You have negative zero velocities, dont use them."
		<< "\nPlease think of the neighbour lists.";
#endif

  for (size_t iDim = 0; iDim < NDIM; ++iDim)
    if ((iDim == GravityDim) && part.testState(Particle::DYNAMIC))
      {
	double adot = Gravity;
	double vdot = vel[GravityDim];
	
	//First check the "upper" boundary that may have no roots
	double rdot = (Gravity < 0) ? rpos[iDim]-width[iDim]: rpos[iDim];
	double arg = vdot * vdot - 2 * rdot * adot;
	double upperRoot1(HUGE_VAL), upperRoot2(HUGE_VAL);
	
	if (arg >= 0)
	  {
	    double t = -(vdot + ((vdot<0) ? -1: 1) * std::sqrt(arg));
	    upperRoot1 = t / adot;
	    upperRoot1 = 2 * rdot / t;
	    if (upperRoot2 < upperRoot1) std::swap(upperRoot2, upperRoot1);
	  }

	
	//Now the lower boundary which always has roots
	rdot = (Gravity < 0) ? rpos[iDim] : rpos[iDim]-width[iDim];
	arg = vdot * vdot - 2 * rdot * adot;
	double lowerRoot1(HUGE_VAL), lowerRoot2(HUGE_VAL);
	if (arg >= 0)
	  {
	    double t = -(vdot + ((vdot<0) ? -1: 1) * std::sqrt(arg));
	    lowerRoot1 = t / adot;
	    lowerRoot2 = 2 * rdot / t;
	    if (lowerRoot2 < lowerRoot1) std::swap(lowerRoot2, lowerRoot1);
	  }

	//Now, if the velocity is "up", and the upper roots exist,
	//then pick the shortest one
	if (!((Gravity < 0) - (vel[GravityDim] > 0)))
	  if (upperRoot1 < time)
	    {
	      time = upperRoot1;
	      retVal = (Gravity < 0) ? (iDim + 1) : -(iDim + 1);
	    }

	//Otherwise its usually the latest lowerRoot
	if (lowerRoot2 < time)
	  {
	    time = lowerRoot2;
	    retVal = (Gravity < 0) ? - (iDim + 1) : (iDim + 1);
	  }
      }  
  else
    {
      double tmpdt = ((vel[iDim] < 0) 
		  ? -rpos[iDim] / vel[iDim] 
		  : (width[iDim] - rpos[iDim]) / vel[iDim]);

      if (tmpdt < time)
	{
	  time = tmpdt;
	  retVal = (vel[iDim] < 0) ? - (iDim + 1) : iDim + 1;
	}
    }

  return retVal;
}

void 
LNewtonianGravity::outputXML(xml::XmlStream& XML) const
{
  XML << xml::attr("Type") 
      << "NewtonianGravity"
      << xml::attr("Gravity") 
      << Gravity / Sim->dynamics.units().unitAcceleration()
      << xml::attr("GravityDimension") 
      << GravityDim
    ;
}

double 
LNewtonianGravity::getPBCSentinelTime(const Particle& part, const double& lMax) const
{
  M_throw() << "Not implemented yet";  
}

std::pair<bool,double>
LNewtonianGravity::getPointPlateCollision(const Particle& part, const Vector& nrw0,
				 const Vector& nhat, const double& Delta,
				 const double& Omega, const double& Sigma,
				 const double& t, bool lastpart) const
{
  M_throw() << "Not implemented yet";
}

double 
LNewtonianGravity::getCylinderWallCollision(const Particle& part, 
				   const Vector& wallLoc, 
				   const Vector& wallNorm,
				   const double& radius) const
{
  M_throw() << "Not implemented yet";
}

double 
LNewtonianGravity::getParabolaSentinelTime(const Particle& part, 
					   unsigned char& passed) const
{
#ifdef DYNAMO_DEBUG
  if (!isUpToDate(part))
    M_throw() << "Particle is not up to date";
#endif
  
  Vector pos(part.getPosition()), vel(part.getVelocity());
  
  Sim->dynamics.BCs().applyBC(pos, vel);
  
  double turningPoint = - vel[GravityDim] / Gravity;
  
  if (turningPoint <= 0)
    {
      passed = true;
      return HUGE_VAL;
    }
  
  return turningPoint;
}

void 
LNewtonianGravity::enforceParabola(const Particle& part) const
{
  updateParticle(part);

  const_cast<Particle&>(part).getVelocity()[GravityDim] = 0.0;
}
