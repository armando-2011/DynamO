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
#include <dynamo/dynamics/eventtypes.hpp>
#include <dynamo/dynamics/interactions/intEvent.hpp>
#include <dynamo/dynamics/globals/globEvent.hpp>
#include <dynamo/dynamics/locals/localEvent.hpp>
#include <dynamo/dynamics/globals/global.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include <queue>

namespace dynamo {
  //Datatype for a single event, stored in lists for each particle
  class intPart
  {
  public:   
    inline intPart():
      dt(HUGE_VAL),
      collCounter2(std::numeric_limits<unsigned long>::max()),
      type(NONE),
      p2(std::numeric_limits<size_t>::max())    
    {}

    inline intPart(const double& ndt, const EEventType& nT) throw():
      dt(ndt),
      type(nT),
      p2(0)
    {}

    inline intPart(const double& ndt, const unsigned long & direction) throw():
      dt(ndt),
      collCounter2(direction),
      type(CELL),
      p2(0)
    {}
  
    inline intPart(const double& ndt, const EEventType& nT, 
		   const size_t& nID2, const unsigned long & nCC2) throw():
      dt(ndt),
      collCounter2(nCC2),
      type(nT),
      p2(nID2)
    {}

    inline intPart(const IntEvent& coll, const unsigned long& nCC2) throw():
      dt(coll.getdt()),
      collCounter2(nCC2),
      type(INTERACTION),
      p2(coll.getParticle2ID())
    {}

    inline intPart(const GlobalEvent& coll) throw():
      dt(coll.getdt()),
      type(GLOBAL),
      p2(coll.getGlobalID())
    {}

    inline intPart(const LocalEvent& coll) throw():
      dt(coll.getdt()),
      type(LOCAL),
      p2(coll.getLocalID())
    {}

    inline bool operator< (const intPart& ip) const throw()
    { return dt < ip.dt; }

    inline bool operator> (const intPart& ip) const throw()
    { return dt > ip.dt; }

    inline void stream(const double& ndt) throw() { dt -= ndt; }

    mutable double dt;
    unsigned long collCounter2;
    EEventType type;
    size_t p2;  
  };

  typedef std::vector<intPart> qType;
  typedef std::priority_queue<intPart, qType, 
			      std::greater<intPart> > pList_q_type;
  class pList: public pList_q_type
  {
  public:
    typedef qType::iterator CRanIt;
    typedef qType::iterator iterator;
    typedef qType::const_iterator const_iterator;
    typedef std::iterator_traits<CRanIt>::difference_type CDiff;
  
    inline iterator begin() { return c.begin(); }
    inline const_iterator begin() const { return c.begin(); }
    inline iterator end() { return c.end(); }
    inline const_iterator end() const { return c.end(); }

    inline void clear()
    { c.clear(); }

    inline bool operator> (const pList& ip) const throw()
    { 
      //If the other is empty this can never be longer
      //If this is empty and the other isn't its always longer
      //Otherwise compare
      return (ip.c.empty()) 
	? false
	: (empty() || (c.front().dt > ip.c.front().dt)); 
    }

    inline bool operator< (const pList& ip) const throw()
    { 
      //If this is empty it can never be shorter
      //If the other is empty its always shorter
      //Otherwise compare
      return (empty()) 
	? false 
	: (ip.c.empty() || (c.front().dt < ip.c.front().dt)); 
    }

    inline double getdt() const 
    { 
      return (c.empty()) ? HUGE_VAL : c.front().dt; 
    }
  
    inline void stream(const double& ndt) throw()
    {
      BOOST_FOREACH(intPart& dat, c)
	dat.dt -= ndt;
    }

    inline void addTime(const double& ndt) throw()
    {
      BOOST_FOREACH(intPart& dat, c)
	dat.dt += ndt;
    }

    inline void push(const intPart& __x)
    {
      c.push_back(__x);
      std::push_heap(c.begin(), c.end(), comp);
    }

    inline void rescaleTimes(const double& scale) throw()
    { 
      BOOST_FOREACH(intPart& dat, c)
	dat.dt *= scale;
    }

    inline void swap(pList& rhs)
    {
      c.swap(rhs.c);
    }
  };
}

namespace std
{
  /*! \brief Template specialisation of the std::swap function for pList*/
  template<> inline void swap(dynamo::pList& lhs, dynamo::pList& rhs)
  { lhs.swap(rhs); }
}
