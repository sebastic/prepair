/*
 Copyright (c) 2009-2013,
 Ken Arroyo Ohori    g.a.k.arroyoohori@tudelft.nl
 Hugo Ledoux         h.ledoux@tudelft.nl
 Martijn Meijers     b.m.meijers@tudelft.nl
 All rights reserved.
 
 // This file is part of prepair: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Licensees holding a valid commercial license may use this file in
 accordance with the commercial license agreement provided with
 the software.
 
 This file is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef POLYGONREPAIR_H

#include "definitions.h"

class PolygonRepair {
public:
    OGRMultiPolygon *repairOddEven(OGRGeometry *geometry);
    OGRMultiPolygon *repairPointSet(OGRGeometry *geometry);
    void removeSmallPolygons(OGRMultiPolygon *outPolygons, double minArea);
    OGRMultiPolygon *isr(OGRGeometry *geometry);
    
private:
    void insertConstraints(Triangulation &triangulation, OGRGeometry *geometry);
    void tagOddEven(Triangulation &triangulation);
    void tagPointSet(Triangulation &triangulation, std::list<std::pair<bool, OGRMultiPolygon *> > &geometries);
    OGRMultiPolygon *reconstruct(Triangulation &triangulation);
    
    std::list<Triangulation::Vertex_handle> *getBoundary(Triangulation::Face_handle face, int edge);
    void printEdges(Triangulation &triangulation);
    void printTriangle(Triangulation::Face_handle triangle);
    void printChain(std::list<Triangulation::Vertex_handle> &chain);
};

#endif