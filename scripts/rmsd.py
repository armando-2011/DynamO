#!/usr/bin/env python

import math
import numpy
import sys
import os
import pickle

xmlstarlet='xml'

def rmsd(crds1, crds2):
  """Returns RMSD between 2 sets of [nx3] numpy array"""
  v, s, w_tr = numpy.linalg.svd(numpy.dot(numpy.transpose(crds1), crds2))
  return numpy.sqrt(max([(sum(sum(crds1 * crds1)) + sum(sum(crds2 * crds2)) - 2.0*sum(s)) / float(numpy.shape(crds1)[0]), 0.0]))

def get_structlist(outputfile):
  cmd = 'bzcat '+outputfile+' | '+xmlstarlet+' sel -t -m \'/OutputData/StructureImages\' -m \'Image\' -m \'Atom\' -v \'@x\' -o "," -v \'@y\' -o \',\' -v \'@z\' -o \':\' -b -n | gawk \'{if (NF) print $0}\''

  structlist = []
  for line in os.popen(cmd).readlines():
    line = line.rstrip(':')
    line = line.rstrip(':\n')
    structlist.append(numpy.array([[float(x) for x in atom.split(',')] for atom in line.split(':')]))

  print "Number of structures is "+str(len(structlist))
  if (len(structlist) > 100):
    print "\nTruncating to 100"
    del structlist[0::5]
    del structlist[0::4]
    del structlist[0::3]
    del structlist[0::2]

  return structlist

def get_temperature(file):
    cmd = 'bzcat '+file+' | '+xmlstarlet+' sel -t -v \'/OutputData/KEnergy/T/@val\''
    return float(os.popen(cmd).read())  

from operator import add

def get_rmsd_data(structlist):
  arrayList = numpy.zeros( (len(structlist),len(structlist)), float)
  for i in range(len(structlist)):    
    for j in range(i+1,len(structlist)):
      arrayList[i][j] = min(rmsd(structlist[i], structlist[j]), rmsd(structlist[i], (structlist[j])[::-1]))
      arrayList[j][i] = arrayList[i][j]

  minimum = 0
  minsum = 1e308
  for i in range(len(structlist)):  
    localsum = sum(arrayList[i])
    if (localsum < minsum):
      minimum = i
      minsum = localsum

  return minimum, minsum / len(structlist), arrayList

import copy

def cluster_analysis(arrayList, length, threshold):
  localCluster = []
  for i in range(len(arrayList)):
    local = []
    count = 0
    for j in range(len(arrayList)):
      if (arrayList[i][j] < length):
        local.append(j)
        count += 1
    localCluster.append([count, local, i])
  
  clusters = []
  maxval = copy.deepcopy(max(localCluster))
  while (maxval[0] > threshold):
    clusters.append(maxval)
    for j in range(len(arrayList)):
      for id in maxval[1]:
        if (id in localCluster[j][1]):
          localCluster[j][1].remove(id)
          localCluster[j][0] -= 1
    maxval = copy.deepcopy(max(localCluster))
    
  return clusters

def print_structure(structure, filename):
  f = open(filename, 'w')
  try:
    length = str(len(structure))
    print >>f, "{VECT 1 "+length+" 0 "
    print >>f, length
    print >>f, "0"
    
    for i in range(len(structure)):
      print >>f, structure[i][0], structure[i][1], structure[i][2]
      
    #print >>f,"1 1 1 1"
    print >>f, "}"
  finally:
    f.close()

def print_vmd_structure(structure, filename):
  f = open(filename, 'w')
  try:
    length = str(len(structure))
    print >>f, length
    print >>f, "RMSD VMD structure"
    for i in range(len(structure)):
      print >>f,"C", structure[i][0]*3.4, structure[i][1]*3.4, structure[i][2]*3.4
  finally:
    f.close()

filedata = []
for file in  sys.argv[1:]:
  print "Processing file "+file
  if (os.path.exists(file+'.rmspickle3')):
    print "Found pickled data, skipping minimisation"
    f = open(file+'.rmspickle3', 'r')
    tempdata = pickle.load(f)
    tempdata.append(file)
    tempdata.append(get_structlist(file))
    filedata.append(tempdata)
    f.close()
  else:
    print "Could not find pickled minimisation data"
    tempdata = [ get_temperature(file), get_rmsd_data(get_structlist(file)) ]

    f = open(file+'.rmspickle3', 'w')
    try:
      pickle.dump(tempdata,f)
    finally:
      f.close()

    tempdata.append(file)
    tempdata.append(get_structlist(file))
    filedata.append(tempdata)


filedata.sort()


for data in filedata:
  f = open(data[2]+'.rmsdhist', 'w')
  g = open(data[2]+'.rmsdarray', 'w')
  try:
    arrayList = numpy.zeros(100, float)
    xmax = 5.0
    delx = xmax / 100.0
    count = 0
    for i in range(len(data[1][2])):
      for j in range(i+1,len(data[1][2])):
        coord = data[1][2][i][j]/delx
        if (coord < 100):
          arrayList[int(coord)] += 1
          count += 1

    for i in range(len(data[1][2])):
      for j in range(len(data[1][2])):
        print >>g, data[1][2][i][j],
      print >>g, "\n",

    for i in range(len(arrayList)):
      print >>f, (i+0.5)*delx, arrayList[i]/float(count)
  finally: 
    f.close()
    g.close()

f = open('rmsdhistsurface.dat', 'w')
try:
  for data in filedata:
    arrayList = numpy.zeros(101, float)
    xmax = 5.0
    delx = xmax / 100.0
    count = 0
    for i in range(len(data[1][2])):
      for j in range(i+1,len(data[1][2])):
        if (int(data[1][2][i][j]/delx) < 100):
          arrayList[int(data[1][2][i][j]/delx)] += 1
          count += 1
    
    for i in range(len(arrayList)):
      print >>f, data[0], (i+0.5)*delx, arrayList[i]/float(count)

    print >>f, " "
finally: 
  f.close()
  
#Outputs the min rmsd and the structure number
f = open('clusters.dat', 'w')
g = open('clusterfraction.dat', 'w')
h = open('avgrmsd.dat', 'w')
try:
  for data in filedata:
    clusters = cluster_analysis(data[1][2], 0.6, 5)
    if (len(filedata)==1):
      print clusters
      for cluster in clusters:
        print "Printing structure number "+str(cluster[2])
        print_structure(data[3][cluster[2]],"Cluster"+str(cluster[2])+".list")
    sum = 0
    for cluster in clusters:
      sum += cluster[0]
      
    print >>f, data[0], len(clusters)
    print >>g, data[0], float(sum) / float(len(data[1][2]))
    print >>h, data[0], data[1][1], data[1][0]
finally:
  f.close()
  g.close()
  h.close()
  