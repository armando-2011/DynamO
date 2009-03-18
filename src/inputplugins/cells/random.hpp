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

#pragma once
#include "cell.hpp"
#include "../../base/is_simdata.hpp"

struct CURandom: public CUCell
{
  CURandom(size_t nN, CVector<> ndimensions, 
	   boost::uniform_01<DYNAMO::baseRNG, Iflt>& RNG,
	   CUCell* nextCell):
    CUCell(nextCell),
    N(nN),
    dimensions(ndimensions),
    uniform_sampler(RNG)
  {}

  size_t N;
  CVector<> dimensions;
  boost::uniform_01<DYNAMO::baseRNG, Iflt>& uniform_sampler;

  virtual std::vector<CVector<> > placeObjects(const CVector<>& centre)
  {
    std::vector<CVector<> > retval;

    for (size_t i(0); i < N; ++i)
      {
	CVector<> position;
	for (int iDim = 0; iDim < NDIM; iDim++)
	  position[iDim] = centre[iDim] - (uniform_sampler() - 0.5) * dimensions[iDim];
	
	
	//Get the next unit cells positions and push them to your list
	BOOST_FOREACH(const CVector<>& vec, uc->placeObjects(position))
	  retval.push_back(vec);
      }

    return retval;    
  }
};