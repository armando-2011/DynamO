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
namespace magnet { namespace xml { class Node; class XmlStream; } }
namespace dynamo { 
  class SimData;
  class Particle;

  class C2Range
  {
  public:
    virtual ~C2Range() {};
 
    virtual bool isInRange(const Particle&, const Particle&) const =0;  
    virtual void operator<<(const magnet::xml::Node& XML) = 0;
  
    static C2Range* getClass(const magnet::xml::Node&, const dynamo::SimData*);

    friend magnet::xml::XmlStream& operator<<(magnet::xml::XmlStream&, const C2Range&);

  protected:
    virtual void outputXML(magnet::xml::XmlStream& XML) const = 0;
  };
}
