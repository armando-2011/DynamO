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
#include <dynamo/dynamics/systems/system.hpp>
#include <dynamo/base/is_simdata.hpp>
#include <dynamo/dynamics/ranges/1range.hpp>
#include <tr1/memory>

namespace dynamo {
  class CSDSMCSpheres: public System
  {
  public:
    CSDSMCSpheres(const magnet::xml::Node& XML, dynamo::SimData*);

    CSDSMCSpheres(dynamo::SimData*, double, double, double, double, std::string, CRange*, CRange*);
  
    virtual void runEvent() const;

    virtual void initialise(size_t);

    virtual void operator<<(const magnet::xml::Node&);

  protected:
    virtual void outputXML(magnet::xml::XmlStream&) const;

    double tstep;
    double chi;
    double d2;
    double diameter;
    mutable double maxprob;
    double e;
    double factor;

    std::tr1::shared_ptr<CRange> range1;
    std::tr1::shared_ptr<CRange> range2;
  };
}
