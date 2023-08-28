#include "Data_Class.h" 

/*****class Data_ClusterData*****/
Data_ClusterData::Data_ClusterData(){
	clusterResult = 1;
}

Data_ClusterData::~Data_ClusterData(){}

/*****class Data_KMeansMeans*****/
Data_KMeansMeans::Data_KMeansMeans(int mN, vector<double> me){
	meansNumber = mN;
	means = me;
}

Data_KMeansMeans::~Data_KMeansMeans(){}	

/*****class Data_MergeCluster*****/
void Data_MergeCluster::computePeriodCount(int MinPeriod){	
	struct tm tm;
	strptime(endtime.c_str(), "%Y/%m/%d %H:%M:%S", &tm);
	time_t end_t = mktime(&tm); 	
	strptime(begtime.c_str(), "%Y/%m/%d %H:%M:%S", &tm);
	time_t beg_t = mktime(&tm); 
	periodCount = difftime(end_t, beg_t)/(MinPeriod*60);	
}

void Data_MergeCluster::ConvertTimeToWeedDay(){
	struct tm tmt;
	strptime(begtime.c_str(), "%Y/%m/%d %H:%M:%S", &tmt);
	time_t beg_t = mktime(&tmt);
	tm * time_w = localtime(&beg_t);
	weekNum = time_w->tm_wday;	//0~6, Sunday~Saturday
}

/*****Data_TODOWMerge*****/
void Data_TODOWMerge::computeProbAndCluNo(){
	map<int, int>::iterator iter;
	int totalcount = 0;
	for(iter=ClusterCount.begin();iter!=ClusterCount.end();iter++){
		totalcount = totalcount + iter->second;	
	}		
	
	int finalCluNo = 0; double maxProb = 0;
	for(iter=ClusterCount.begin();iter!=ClusterCount.end();iter++){
		double Prob = (double)iter->second/(double)totalcount;
		if(Prob>maxProb){
			finalCluNo = iter->first;
			maxProb = Prob; 
		}	
	}
	TODOWCCluNo = finalCluNo;
	CluNoProb = maxProb;
}

/*****Data_TODPeriod*****/
void Data_TODPeriod::computePeriodCount(int MinPeriod){		
	struct tm tm;
	strptime(endtime.c_str(), "%H:%M:%S", &tm);
	time_t end_t = mktime(&tm); 	
	strptime(begtime.c_str(), "%H:%M:%S", &tm);
	time_t beg_t = mktime(&tm); 
	periodCount = difftime(end_t, beg_t)/(MinPeriod*60);	
}









