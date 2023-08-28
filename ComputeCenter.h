#ifndef _COMPUTECENTER_H_ 
#define _COMPUTECENTER_H_

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <iterator>
#include <Python.h>
#include <algorithm>
#include <sys/stat.h>

#include "Data_Class.h" 

using namespace std;

class ComputeCenter{
	private:
		//parameter
		int MinPeriod; int periodLenLimit;
		double bigCarDis; double CarDis; double motoCarDis;
		string ProjectFolder;
		vector<int> dirVertor;
		string begTime;
		string endTime;
		
		map<string, string> SpecialDay;
		map<string, double> DisChargeTimeData;
		vector<Data_ClusterData> ClusterData;
		map<int, Data_KMeansMeans> KMeansMeans;
		vector<Data_MergeCluster> MergeClusterData;
		map<int, Data_DOWPeriod> TODOW_PerResult;		
		
		PyObject *pModule, *pFunc, *sysPath, *modulPath;		
		
		//function
		PyObject* prepareClusterData();
		void PrepareClusterNoMapID();
		void parseClusterPyObjReturn(PyObject* incoming);
		void initialCPython(char *pythonfile, char *pythonPackage);
		void OutputClusterResult(string folder);
		void OutputMergeResult(string folder);
		void OutputMergeResultByWeek(string folder);
		
		//tool box
		string convertSpacePadded(int integer);		
		
	public:			
		//function
		ComputeCenter(char *pythonfile, char *pythonPackage, string folder, string dirV, string begT, string endT);
		~ComputeCenter();
		
		void checkOutputFolder(string OutputFolder);
		void readSpecialDayFile(string filename);
		void readFile(string filename);
		void DoCluster(int clusterNum);		
		void MergeClusterResult();
		void MergeDataByWeek();
		void OutPutProcessResult(string content);
		
		//tool box
		vector<string> split(string str, string pattern);
		string moveMinutes(string basepoint, int step);
		string moveMinutesOnlyMin(string basepoint, int step);
		string intToString(int integer);
};


#endif



