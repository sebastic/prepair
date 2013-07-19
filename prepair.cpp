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

#include "PolygonRepair.h"

//-- minimum size of a polygon in the output (smaller ones are not returned)
//-- can be changed with --min flag
double minArea = 0;
double isrTolerance = 0;
bool wktOut = false;
bool shpOut = false;
bool computeRobustness = false;
bool pointSet = false;

void usage();
bool savetoshp(OGRMultiPolygon* multiPolygon);

int main (int argc, const char * argv[]) {
    
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage();
        return(0);
    }
    
    OGRGeometry *geometry;
    
    for (int argNum = 1; argNum < argc; ++argNum) {
        //-- whether to compute the robustness
        if (strcmp(argv[argNum], "--robustness") == 0) {
            computeRobustness = true;
        }
        
        //-- mininum area to keep in output
        else if (strcmp(argv[argNum], "--minarea") == 0) {
            minArea = atof(argv[argNum+1]);
            ++argNum;
        }
        
        //-- ISR snapping tolerance
        else if (strcmp(argv[argNum], "--isr") == 0) {
            isrTolerance = atof(argv[argNum+1]);
            ++argNum;
            // TODO : scale dataset if tolerance < 1.0
        }
        
        //-- reading from WKT passed directly
        else if (strcmp(argv[argNum], "--wkt") == 0) {
            unsigned int bufferSize = 100000000;
            char *inputWKT = (char *)malloc(bufferSize*sizeof(char *));
            strcpy(inputWKT, argv[argNum+1]);
            ++argNum;
            OGRGeometryFactory::createFromWkt(&inputWKT, NULL, &geometry);
            if (geometry == NULL) {
                std::cout << "Error: WKT is not valid" << std::endl;
                return 1;
            } wktOut = true;
        }
        
        //-- reading from WKT stored in first line of a text file
        else if (strcmp(argv[argNum], "-f") == 0) {
            unsigned int bufferSize = 100000000;
            char *inputWKT = (char *)malloc(bufferSize*sizeof(char *));
            if (argNum + 1 <= argc - 1 && argv[argNum+1][0] != '-') {
                std::ifstream infile(argv[argNum+1], std::ifstream::in);
                infile.getline(inputWKT, bufferSize);
                ++argNum;
            } else {
                std::cerr << "Error: Missing input file name." << std::endl;
                return 1;
            }
            OGRGeometryFactory::createFromWkt(&inputWKT, NULL, &geometry);
            if (geometry == NULL) {
                std::cout << "Error: WKT is not valid" << std::endl;
                return 1;
            } wktOut = true;
        }
        
        //-- reading from a shapefile
        else if (strcmp(argv[argNum], "--shp") == 0) {
            OGRRegisterAll();
            OGRDataSource *dataSource = OGRSFDriverRegistrar::Open(argv[argNum+1], false);
            ++argNum;
            if (dataSource == NULL) {
                std::cerr << "Error: Could not open file." << std::endl;
                return false;
            }
            OGRLayer *dataLayer = dataSource->GetLayer(0); //-- get first layer
            dataLayer->ResetReading();
            //-- Reads all features in this layer
            OGRFeature *feature;
            //-- get first feature
            if (dataLayer->GetFeatureCount(true) > 1)
                std::cout << "Reading only the first feature in the shapefile." << std::endl;
            feature = dataLayer->GetNextFeature();
            if (feature->GetGeometryRef()->getGeometryType() == wkbPolygon) {
                geometry = static_cast<OGRPolygon *>(feature->GetGeometryRef());feature->GetGeometryRef();
                shpOut = true;
            }
            else {
                std::cout << "First feature ain't a POLYGON." << std::endl;
                return(0);
            }
            
        }
        else {
            usage();
            return(0);
        }
    }
    
    PolygonRepair prepair;
    OGRGeometry *snappedGeometry;
    if (isrTolerance != 0) {
        snappedGeometry = prepair.isr(geometry, isrTolerance);
    } else {
        snappedGeometry = geometry;
    }
    
    OGRMultiPolygon *outPolygons = prepair.repairPointSet(snappedGeometry);
    
    if (minArea > 0) {
        prepair.removeSmallPolygons(outPolygons, minArea);
    }
    
    //-- output well known text
    if (wktOut) {
        char *outputWKT;
        outPolygons->exportToWkt(&outputWKT);
        std::cout << outputWKT << std::endl;
    }
    
    //-- save to a shapefile
    else if (shpOut) {
        savetoshp(outPolygons);
    }
    
    //-- compute robustness
    if (computeRobustness == true)
        std::cout << "Robustness of input polygon: " << sqrt(prepair.computeRobustness()) <<std::endl;
    
    return 0;
}

void usage() {
    std::cout << "=== prepair Help ===\n" << std::endl;
    std::cout << "Usage:   prepair --wkt 'POLYGON(...)'" << std::endl;
    std::cout << "OR" << std::endl;
    std::cout << "Usage:   prepair -f infile.txt (infile.txt must contain one WKT on the 1st line)" << std::endl;
    std::cout << "OR" << std::endl;
    std::cout << "Usage:   prepair --shp infile.shp (first polygon of infile.shp is processed)" << std::endl;
    
}


bool savetoshp(OGRMultiPolygon* multiPolygon) {
    const char *driverName = "ESRI Shapefile";
    OGRRegisterAll();
	OGRSFDriver *driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(driverName);
	if (driver == NULL) {
		std::cout << "\tError: OGR Shapefile driver not found." << std::endl;
		return false;
	}
	OGRDataSource *dataSource = driver->Open("out.shp", false);
	if (dataSource != NULL) {
		std::cout << "\tOverwriting file..." << std::endl;
		if (driver->DeleteDataSource(dataSource->GetName())!= OGRERR_NONE) {
			std::cout << "\tError: Couldn't erase file with same name." << std::endl;
			return false;
		} OGRDataSource::DestroyDataSource(dataSource);
	}
	std::cout << "\tWriting file... " << std::endl;
	dataSource = driver->CreateDataSource("out.shp", NULL);
	if (dataSource == NULL) {
		std::cout << "\tError: Could not create file." << std::endl;
		return false;
	}
	OGRLayer *layer = dataSource->CreateLayer("polygons", NULL, wkbPolygon, NULL);
	if (layer == NULL) {
		std::cout << "\tError: Could not create layer." << std::endl;
		return false;
	}
    OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
    // feature->SetField("Name", szName);
    feature->SetGeometry(multiPolygon);
    if (layer->CreateFeature(feature) != OGRERR_NONE) {
        std::cout << "\tError: Could not create feature." << std::endl;
    }
    OGRFeature::DestroyFeature(feature);
    OGRDataSource::DestroyDataSource(dataSource);
    return true;
}