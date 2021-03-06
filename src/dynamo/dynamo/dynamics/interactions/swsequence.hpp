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

#pragma once

#include <dynamo/dynamics/interactions/captures.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/interactions/representations/spherical.hpp>
#include <vector>
#include <set>

namespace dynamo {
  class ISWSequence: public ISingleCapture, public Interaction, public SphericalRepresentation
  {
  public:
    template<class T1, class T2, class T3>
    ISWSequence(dynamo::SimData* tmp, T1 d, T2 l, T3 e, 
		std::vector<size_t> seq, C2Range* nR):
      Interaction(tmp,nR),
      _diameter(Sim->_properties.getProperty
		(d, Property::Units::Length())),
      _lambda(Sim->_properties.getProperty
	      (l, Property::Units::Dimensionless())),
      _unitEnergy(Sim->_properties.getProperty
		  (1.0, Property::Units::Energy())),
      _e(Sim->_properties.getProperty
	 (e, Property::Units::Dimensionless())),
      sequence(seq) 
    {
      std::set<size_t> letters;
    
      BOOST_FOREACH(const size_t& id, seq)
	if (letters.find(id) == letters.end())
	  //Count the letter
	  letters.insert(id);
    
      alphabet.resize(letters.size());
    
      BOOST_FOREACH(std::vector<double>& vec, alphabet)
	vec.resize(letters.size(), 0.0);
    }

    ISWSequence(const magnet::xml::Node&, dynamo::SimData*);
  
    void operator<<(const magnet::xml::Node&);

    virtual size_t spheresPerParticle() const { return 1; }
    virtual double getDiameter(size_t ID, size_t subID) const;
    virtual Vector getPosition(size_t ID, size_t subID) const;

    virtual double getExcludedVolume(size_t) const;

    virtual double maxIntDist() const;

    virtual double getInternalEnergy() const;

    virtual double getInternalEnergy(const Particle&, const Particle&) const;

    virtual void checkOverlaps(const Particle&, const Particle&) const;

    virtual bool captureTest(const Particle&, const Particle&) const;

    virtual void initialise(size_t);

    virtual IntEvent getEvent(const Particle&, const Particle&) const;
  
    virtual void runEvent(const Particle&, const Particle&, const IntEvent&) const;
  
    virtual void outputXML(magnet::xml::XmlStream&) const;

    std::vector<size_t>& getSequence() { return sequence; }

    std::vector<std::vector<double> >& getAlphabet() { return alphabet; }

  protected:
    std::tr1::shared_ptr<Property> _diameter;
    std::tr1::shared_ptr<Property> _lambda;
    //!This class is used to track how the energy scale changes in the system
    std::tr1::shared_ptr<Property> _unitEnergy;
    std::tr1::shared_ptr<Property> _e;
  
    std::vector<size_t> sequence;
    std::vector<std::vector<double> > alphabet;
  };
}
