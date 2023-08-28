//============================================================================
// Name        : AITimePerCluster.cpp
// Author      : yccho
// Version     :
// Copyright   : Your copyright notice
// Description : Time Period Cluster with AI Data in C++, Ansi-style, handle ONE node
// Compile     : compile.sh
//============================================================================

#include <iostream>
#include <string>

#include "ComputeCenter.h"

using namespace std;

int main(int argc, char *argv[]) {		
	if (argc < 10) {
		fprintf(stderr,"Usage: pythonfile name, python package, direct No. ,cluster Number, begDay, begTime, endDay, endTime, runFolder\n");
		return 1;
	}		
	
	const clock_t begin_time = clock();	
	string dirVector(argv[3]);
	int clusterNum = atoi(argv[4]);
	string begYMD(argv[5]);	string begHMS(argv[6]);
	string endYMD(argv[7]);	string endHMS(argv[8]);
	string runFolder(argv[9]);
		
	ComputeCenter cc(argv[1], argv[2], runFolder, dirVector, begYMD+" "+begHMS, endYMD+" "+endHMS);
	cc.checkOutputFolder(runFolder+"ClusterResult/");
	cc.readSpecialDayFile(runFolder+"SpecialDay.csv");
	cc.readFile(runFolder+"rawData.csv");
	cc.DoCluster(clusterNum);	
	cc.MergeClusterResult();
	cc.MergeDataByWeek();
	cc.OutPutProcessResult("0");
	cout<<"finish!! Compute Time: "<<float(clock() - begin_time)/CLOCKS_PER_SEC<<" secs"<<endl;	
	
	return 0;
}












