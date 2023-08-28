#ifndef _DATA_CLASS_H_ 
#define _DATA_CLASS_H_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <iterator>

using namespace std;

class Data_ClusterData{
	public:
		//parameter
		string time;
		vector<double> disChargeTime;
		int clusterResult;
		
		//function
		Data_ClusterData();
		~Data_ClusterData();		
};

class Data_KMeansMeans{
	public:
		//parameter		
		int meansNumber;
		int sortCluNumber;
		vector<double> means;		
		
		//function
		Data_KMeansMeans(int mN, vector<double> me);
		~Data_KMeansMeans();		
};

class Data_MergeCluster{
	public:
		//parameter		
		string begtime;
		string endtime;
		int clusterResult;	
		int periodCount;
		int weekNum;
		
		//function
		void computePeriodCount(int MinPeriod);
		void ConvertTimeToWeedDay();
};

class Data_TODOWMerge{
	public:
		//parameter		
		map<int, int> ClusterCount;
		int TODOWCCluNo;
		double CluNoProb;
		
		//function
		void computeProbAndCluNo();
};

class Data_TODPeriod{
	public:
		//parameter		
		string begtime;
		string endtime;
		int clusterResult;	
		int periodCount;
		double CluNoProb;
		
		//function
		void computePeriodCount(int MinPeriod);
};

class Data_DOWPeriod{
	public:
		//parameter	
		vector<Data_TODPeriod> TOD_PerResult;
};


#endif








