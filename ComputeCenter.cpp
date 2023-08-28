#include "ComputeCenter.h" 

ComputeCenter::ComputeCenter(char *pythonfile, char *pythonPackage, string folder, string dirV, string begT, string endT){
	MinPeriod = 5;			//5 minutes data
	periodLenLimit = 3;		//5*3 minutes can merge to 1 period
	bigCarDis = 0.1; CarDis = 0.5; motoCarDis = 2;
	ProjectFolder = folder;	
	begTime = begT;
	endTime = endT;
	initialCPython(pythonfile, pythonPackage);
	
	vector<string> vtemp = split(dirV, ",");
	for(unsigned int i=0;i<vtemp.size();i++){
		dirVertor.push_back(atoi(vtemp[i].c_str()));
	}
}

ComputeCenter::~ComputeCenter(){
	Py_DECREF(pFunc);
	Py_DECREF(pModule);
	Py_DECREF(sysPath);	
	Py_DECREF(modulPath);
	
	if (Py_FinalizeEx() < 0) {
		cout<<"Py_FinalizeEx Error!!"<<endl;
	}
}

void ComputeCenter::initialCPython(char *pythonfile, char *pythonPackage){
	/*** important!! ***/
	Py_Initialize(); 
	wchar_t argvPy[1];
	wchar_t *pos = argvPy;
	PySys_SetArgv(0, &pos);
	/*** important!! ***/
	
	modulPath = PyUnicode_DecodeFSDefault(pythonPackage);
	sysPath = PySys_GetObject("path");	
	PyList_Insert(sysPath, 0, modulPath);	
	PyObject *pName = PyUnicode_DecodeFSDefault(pythonfile);
	
	if (PyErr_Occurred()) {
		cerr << "pName decoding failed." << endl;
		OutPutProcessResult("pName decoding failed.");
		exit(0);		
	}else{
		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		if (pModule != NULL) {
			pFunc = PyObject_GetAttrString(pModule, "clusterByKmeans");	
			if (pFunc && PyCallable_Check(pFunc)){
				cout<<"CPython Ready!!"<<endl;
			}else{
				if (PyErr_Occurred())
					PyErr_Print();
				fprintf(stderr, "Cannot find python function \"%s\"\n", "clusterByKmeans");	
				OutPutProcessResult("Cannot find python function.");
				exit(0);
			}
		}else{
			PyErr_Print();
			fprintf(stderr, "Failed to load \"%s\"\n", pythonfile);
			string pyString(pythonfile);
			string error = "Failed to load "+pyString+".";
			OutPutProcessResult(error);
			exit(0);
		}
	}
}

void ComputeCenter::checkOutputFolder(string OutputFolder){
	struct stat buffer;
	bool check = (stat(OutputFolder.c_str(), &buffer) == 0);	
	if(!check){
		string com = "mkdir -p "+OutputFolder;
		const int dir_err = system(com.c_str());
		if (-1 == dir_err){
			cout<<"Error creating directory!n"<<endl;
			OutPutProcessResult("Error creating directory.");
			exit(0);
		}		
	}
}

void ComputeCenter::readSpecialDayFile(string filename){		
	ifstream infile(filename.c_str());
	if(!infile.fail()){
		string row;		
		while (getline(infile, row)){
			istringstream iss(row);				
			SpecialDay.insert(make_pair(row, row));
		}	
	}
	infile.close();
}

void ComputeCenter::readFile(string filename){	
	ifstream infile(filename.c_str());
	if(!infile.fail()){
		string row;
		int count = 0;
		
		while (getline(infile, row)){
			if(count!=0){
				istringstream iss(row);			
				vector<string> temp = split(row, ",");		
				string datetime = temp[3]+":00";	
				string key = temp[1]+"_"+datetime;
				double bigCarVolume = atof(temp[5].c_str())+atof(temp[6].c_str())+atof(temp[7].c_str());	//5-7, bigCar-left, bigCar-straight, bigCar-right
				double carVolume = atof(temp[8].c_str())+atof(temp[9].c_str())+atof(temp[10].c_str());		//8-10, car-left, car-straight, car-right
				double motoVolume = atof(temp[11].c_str())+atof(temp[12].c_str())+atof(temp[13].c_str());	//11-13, moto-left, moto-straight, moto-right
				double disT_moto = 0; if(motoVolume>0 && motoCarDis>0) disT_moto = motoVolume/motoCarDis;
				double disT_scar = 0; if(carVolume>0 && CarDis>0) disT_scar = carVolume/CarDis;
				double disT_bcar = 0; if(bigCarVolume>0 && bigCarDis>0) disT_bcar = bigCarVolume/bigCarDis;
				double disT_car = disT_scar+disT_bcar; 
				double disChargeTime = disT_car; if(disT_moto>disT_car) disChargeTime = disT_moto;
				//cout<<key<<" , "<<carVolume<<" , "<<motoVolume<<" , "<<bigCarVolume<<" , "<<disChargeTime<<" , "<<disT_moto<<" , "<<disT_car<<endl;
				DisChargeTimeData.insert(make_pair(key, disChargeTime));			
			}
			count++;
		}		
		infile.close();	
	}else{
		cout<<"readFile Error!!"<<endl;
		OutPutProcessResult("readFile Error.");
		infile.close();
		exit(0);
	}	
}

void ComputeCenter::DoCluster(int clusterNum){	
	PyObject *pArgs, *pValue;
	
	//input data to python
	pArgs = PyTuple_New(2);
	PyObject* listObj = prepareClusterData();
	PyTuple_SetItem(pArgs, 0, listObj);
	pValue = PyLong_FromLong(clusterNum); 	
	PyTuple_SetItem(pArgs, 1, pValue); 
	
	//return data from python
	pValue = PyObject_CallObject(pFunc, pArgs);
	Py_DECREF(pArgs);			
	
	if (pValue != NULL) {					
		parseClusterPyObjReturn(pValue);
		Py_DECREF(pValue);
	}else{					
		PyErr_Print();
		fprintf(stderr,"Call failed\n");	
		OutPutProcessResult("Call Python failed.");
		exit(0);
	}					
	Py_DECREF(listObj);	
	PrepareClusterNoMapID();
	OutputClusterResult(ProjectFolder+"ClusterResult/");
}

PyObject* ComputeCenter::prepareClusterData(){
	string nowTime = begTime;
	map<string, double>::iterator iter_dis;
	bool speDayEmpty = false;	if(SpecialDay.size()>0) speDayEmpty = true;	
		
	while(nowTime.compare(endTime)){		
		if(speDayEmpty){
			string day = split(nowTime, " ")[0];
			map<string, string>::iterator d_i = SpecialDay.find(day);			
			if(d_i==SpecialDay.end()){				
				bool dirCheck = false;
				vector<double> discharge;		
				for(unsigned int d=0;d<dirVertor.size();d++){
					string dir = intToString(dirVertor[d]);
					string key = dir+"_"+nowTime;
					iter_dis = DisChargeTimeData.find(key);
					if(iter_dis!=DisChargeTimeData.end()){				
						discharge.push_back(iter_dis->second);
					}else{
						dirCheck = true;	
						break;
					}
				}					
				if(!dirCheck){
					Data_ClusterData obj;
					obj.time = nowTime; obj.disChargeTime = discharge;
					ClusterData.push_back(obj);
				}
			}
		}else{			
			bool dirCheck = false;
			vector<double> discharge;		
			for(unsigned int d=0;d<dirVertor.size();d++){
				string dir = intToString(dirVertor[d]);			
				string key = dir+"_"+nowTime;			
				iter_dis = DisChargeTimeData.find(key);
				if(iter_dis!=DisChargeTimeData.end()){				
					discharge.push_back(iter_dis->second);
				}else{
					dirCheck = true;	
					break;
				}
			}	
			if(!dirCheck){
				Data_ClusterData obj;
				obj.time = nowTime; obj.disChargeTime = discharge;
				ClusterData.push_back(obj);
			}	
		}
		nowTime = moveMinutes(nowTime, MinPeriod);
	}
	DisChargeTimeData.clear();
	
	PyObject* listObj;
	if(ClusterData.size()>0){
		listObj = PyList_New(ClusterData.size());
		if(!listObj) cout<<"prepareClusterData Error, Unable to allocate memory for Python list!"<<endl;	
		for(unsigned int i=0;i<ClusterData.size();i++){	
			Data_ClusterData *obj = &ClusterData[i];		
			PyObject* pointList = PyList_New(obj->disChargeTime.size());	
			for(unsigned int j=0;j<obj->disChargeTime.size();j++){			
				PyObject *data = PyFloat_FromDouble(obj->disChargeTime[j]);				
				PyList_SET_ITEM(pointList, j, data);			
			}
			PyList_SET_ITEM(listObj, i, pointList);		
		}	
	}else{
		cout<<"ClusterData size < 0, on ComputeCenter::prepareClusterData()!!"<<endl;
		OutPutProcessResult("ClusterData size < 0, on ComputeCenter::prepareClusterData().");
		exit(0);
	}	
	return listObj;
}		

void ComputeCenter::parseClusterPyObjReturn(PyObject* incoming){	
	if(PyTuple_Check(incoming)){			
		PyObject *clus_result = PyTuple_GetItem(incoming, 0);
		PyObject *kmeans_means = PyTuple_GetItem(incoming, 1);
				
		for(Py_ssize_t i = 0; i < PyList_Size(clus_result); i++){
			PyObject *value = PyList_GetItem(clus_result, i);
			int clusterNum = PyLong_AsLong(value);
			ClusterData[i].clusterResult = clusterNum;						
		}				
		
		for(Py_ssize_t i = 0; i < PyList_Size(kmeans_means); i++){
			PyObject *value = PyList_GetItem(kmeans_means, i);
			vector<double> means;
			for(Py_ssize_t j = 0; j < PyList_Size(value); j++){
				PyObject *point = PyList_GetItem(value, j);
				double p = PyFloat_AsDouble(point);
				means.push_back(p);
			}
			Data_KMeansMeans obj((int)i, means);
			KMeansMeans.insert(make_pair((int)i, obj));
		}		
		Py_DECREF(clus_result); Py_DECREF(kmeans_means); 
	}else{
		cout<<"Error, python return not a tuple!!"<<endl;
	}
}

void ComputeCenter::PrepareClusterNoMapID(){
	map<double, int> temp; map<double, int>::iterator t_i;
	map<int, Data_KMeansMeans>::iterator iter;	
	for(iter=KMeansMeans.begin();iter!=KMeansMeans.end();iter++){
		Data_KMeansMeans *obj = &iter->second;
		double total = 0;
		for(unsigned int i=0;i<obj->means.size();i++){
			total = total + obj->means[i];
		}
		t_i = temp.find(total);
		if(t_i!=temp.end()){
			total = total + 0.1;
			temp.insert(make_pair(total, obj->meansNumber));
		}else{
			temp.insert(make_pair(total, obj->meansNumber));
		}			
	}
	
	map<int, int> KmeansMap;
	int newNo = 0;
	for(t_i=temp.begin();t_i!=temp.end();t_i++){
		KmeansMap.insert(make_pair(t_i->second, newNo));
		//cout<<newNo<<" , "<<t_i->second<<" , "<<t_i->first<<endl;
		newNo++;
	}	
	//cout<<"======="<<endl;
	
	for(iter=KMeansMeans.begin();iter!=KMeansMeans.end();iter++){
		Data_KMeansMeans *obj = &iter->second;
		int meansNumber = obj->meansNumber;			
		obj->sortCluNumber = KmeansMap.find(meansNumber)->second;
		//cout<<meansNumber<<" , "<<obj->sortCluNumber<<endl;
	}
}

void ComputeCenter::OutputClusterResult(string folder){
	string cluResultFN = folder+"ClusterResult.csv";
	char * f = new char[cluResultFN.length()+1];
	strcpy(f, cluResultFN.c_str());		
	fstream fp;	fp.open(f, ios::out);
	if(!fp){
		cout<<"Fail to open output data file: "<<f<<endl;
	}
	
	/*****insert Column Name*****/
	fp<<"DateTime,";
	for(unsigned int d=0;d<dirVertor.size();d++) fp<<"Dir_"<<dirVertor[d]<<",";
	fp<<"MaxDisTime,ClusterNo,SortClusNo,";
	for(unsigned int d=0;d<dirVertor.size();d++){
		if(d==(dirVertor.size()-1)){
			fp<<"Center_"<<dirVertor[d];
		}else{
			fp<<"Center_"<<dirVertor[d]<<",";
		}
	} 
	fp<<endl;
	/*****insert Column Name*****/	
	
	for(unsigned int i=0;i<ClusterData.size();i++){
		fp<<ClusterData[i].time<<",";
		for(unsigned int j=0;j<ClusterData[i].disChargeTime.size();j++){
			fp<<ClusterData[i].disChargeTime[j]<<",";			
		}
		double maxDisTime = *max_element(ClusterData[i].disChargeTime.begin(),ClusterData[i].disChargeTime.end());		
		fp<<maxDisTime<<","<<ClusterData[i].clusterResult<<",";
		int cluR = ClusterData[i].clusterResult;		
		map<int, Data_KMeansMeans>::iterator km_i = KMeansMeans.find(cluR);
		Data_KMeansMeans *k_obj = &km_i->second;
		fp<<k_obj->sortCluNumber<<",";
		for(unsigned int j=0;j<k_obj->means.size();j++){
			if(j==k_obj->means.size()-1){
				fp<<k_obj->means[j];				
			}else{
				fp<<k_obj->means[j]<<",";
			}
		}
		fp<<endl;
	}	
	fp.close();
	
	string cluResult_meansFN = folder+"ClusterResult_means.csv";
	char * f_means = new char[cluResult_meansFN.length()+1];
	strcpy(f_means, cluResult_meansFN.c_str());		
	fstream fp_means; fp_means.open(f_means, ios::out);
	if(!fp_means){
		cout<<"Fail to open output data file: "<<f_means<<endl;
	}
	
	/*****insert Column Name*****/
	fp_means<<"CenterNo,SortClusNo,";	
	for(unsigned int d=0;d<dirVertor.size();d++) fp_means<<"Dir_"<<dirVertor[d]<<",";	
	fp_means<<endl;
	/*****insert Column Name*****/	
	map<int, Data_KMeansMeans>::iterator k_i;
	for(k_i=KMeansMeans.begin();k_i!=KMeansMeans.end();k_i++){
		Data_KMeansMeans *kObj = &k_i->second;	
		fp_means<<kObj->meansNumber<<","<<kObj->sortCluNumber;
		for(unsigned int j=0;j<kObj->means.size();j++){			
			fp_means<<","<<kObj->means[j];			
		}
		fp_means<<endl;
	}
	fp_means.close();	
}

void ComputeCenter::MergeClusterResult(){	
	vector<Data_MergeCluster> temp;	vector<Data_MergeCluster> tempMerge;
	int lastCluResult = 0;	int equalCount = 1;
	string perBegTime;	string perEndTime;	
	for(unsigned int i=0;i<ClusterData.size();i++){
		string time = ClusterData[i].time;
		int clusterResult = ClusterData[i].clusterResult;
		//cout<<time<<" , "<<clusterResult<<" , "<<lastCluResult<<endl;
		
		if(i==0){			
			perBegTime = time;	perEndTime = time;
		}else{			
			if(clusterResult==lastCluResult){				
				if(i==(ClusterData.size()-1)){
					perEndTime = time;
					Data_MergeCluster obj; 
					obj.begtime = perBegTime;	obj.endtime = perEndTime;
					obj.clusterResult = lastCluResult;	obj.periodCount = equalCount;				
					temp.push_back(obj);
				}
				equalCount++;
			}else{
				perEndTime = time;
				Data_MergeCluster obj; 
				obj.begtime = perBegTime;	obj.endtime = perEndTime;
				obj.clusterResult = lastCluResult;	obj.periodCount = equalCount;				
				temp.push_back(obj);
				perBegTime = time;
				equalCount = 1;
			}
		}
		lastCluResult = clusterResult;	
	}
	
	int tempCount = 0;
	for(unsigned int i=0;i<temp.size();i++){
		Data_MergeCluster obj = temp[i];
		//cout<<i<<" , "<<obj.begtime<<" , "<<obj.endtime<<" , "<<obj.clusterResult<<" , "<<obj.periodCount*MinPeriod<<endl;
		
		if(i==0){
			tempMerge.push_back(obj);			
		}else{
			int periodCount = obj.periodCount;
			if(periodCount>periodLenLimit){						
				Data_MergeCluster *lastObj = &tempMerge[tempMerge.size()-1];
				lastObj->endtime = obj.begtime;
				lastObj->periodCount = lastObj->periodCount+tempCount;				
				tempMerge.push_back(obj);
				tempCount = 0;
			}else{
				tempCount = tempCount + periodCount;				
			}	
		}			
	}
	
	for(unsigned int i=0;i<tempMerge.size();i++){
		Data_MergeCluster obj = tempMerge[i];	
		obj.ConvertTimeToWeedDay();
		//cout<<i<<" , "<<obj.begtime<<" , "<<obj.endtime<<" , "<<obj.weekNum<<" , "<<obj.clusterResult<<" , "<<obj.periodCount*MinPeriod<<endl;
		
		if(i==0){			
			MergeClusterData.push_back(obj);
		}else{
			Data_MergeCluster *lastObj = &MergeClusterData[MergeClusterData.size()-1];
			if(obj.clusterResult==lastObj->clusterResult){
				lastObj->endtime = obj.endtime;
				lastObj->periodCount = lastObj->periodCount + obj.periodCount;
				string bd = split(lastObj->begtime," ")[0];	string ed = split(lastObj->endtime," ")[0];
				if(bd.compare(ed)){
					string midleDayTime = ed+" 00:00:00";
					Data_MergeCluster newObj;
					newObj.begtime = midleDayTime; newObj.endtime = lastObj->endtime;
					newObj.clusterResult = lastObj->clusterResult;	
					newObj.computePeriodCount(MinPeriod);
					newObj.ConvertTimeToWeedDay();				
					
					lastObj->endtime = midleDayTime;	
					lastObj->computePeriodCount(MinPeriod);		
					MergeClusterData.push_back(newObj);
				}				
			}else{			
				string bd = split(obj.begtime," ")[0];	string ed = split(obj.endtime," ")[0];
				if(bd.compare(ed)){
					string midleDayTime = ed+" 00:00:00";
					Data_MergeCluster newObj = obj;
					newObj.endtime = midleDayTime;
					newObj.computePeriodCount(MinPeriod);					
					MergeClusterData.push_back(newObj);
					
					obj.begtime = midleDayTime;
					obj.computePeriodCount(MinPeriod);
					obj.ConvertTimeToWeedDay();
				}				
				MergeClusterData.push_back(obj);					
			}
		}		
	}	
	ClusterData.clear();
	OutputMergeResult(ProjectFolder+"ClusterResult/");	
}

void ComputeCenter::OutputMergeResult(string folder){
	string MergeData = folder+"MergeResult.csv";
	char * merge = new char[MergeData.length()+1];
	strcpy(merge, MergeData.c_str());		
	fstream f_merge; f_merge.open(merge, ios::out);
	if(!f_merge){
		cout<<"Fail to open output data file: "<<merge<<endl;
	}
	
	/*****insert Column Name*****/
	f_merge<<"No,BeginTime,EndTime,WeekNumber,periodLength(Min),ClusterNo,SortClusNo,";	
	for(unsigned int d=0;d<dirVertor.size();d++){
		if(d==(dirVertor.size()-1)){
			f_merge<<"Center_"<<dirVertor[d];
		}else{
			f_merge<<"Center_"<<dirVertor[d]<<",";
		}
	} 
	f_merge<<endl;	
	
	/*****insert Column Name*****/
	for(unsigned int i=0;i<MergeClusterData.size();i++){
		Data_MergeCluster *obj = &MergeClusterData[i];
		f_merge<<i<<","<<obj->begtime<<","<<obj->endtime<<","<<obj->weekNum<<"," <<obj->periodCount*MinPeriod<<","<<obj->clusterResult<<",";	
		int cluR = obj->clusterResult;
		map<int, Data_KMeansMeans>::iterator km_i = KMeansMeans.find(cluR);
		Data_KMeansMeans *k_obj = &km_i->second;
		f_merge<<k_obj->sortCluNumber;
		for(unsigned int j=0;j<k_obj->means.size();j++){			
			f_merge<<","<<k_obj->means[j];
			
		}
		f_merge<<endl;		
		//cout<<i<<" , "<<obj->begtime<<" , "<<obj->endtime<<" , "<<obj->weekNum<<" , "<<obj->clusterResult<<" , "<<obj->periodCount*MinPeriod<<endl;
	}
	f_merge.close();
}

void ComputeCenter::MergeDataByWeek(){
	map<string, Data_TODOWMerge> TODOWResult; map<string, Data_TODOWMerge>::iterator iter;
	for(unsigned int i=0;i<MergeClusterData.size();i++){
		Data_MergeCluster *obj = &MergeClusterData[i];
		string begtime = obj->begtime;
		int periodCount = obj->periodCount;		
		int weekNum = obj->weekNum;
		int clusterResult = obj->clusterResult;		
		for(unsigned int t=0;t<periodCount;t++){
			//cout<<weekNum<<" , "<<begtime<<" , "<<split(begtime, " ")[1]<<" , "<<clusterResult<<endl;
			string time = split(begtime, " ")[1];	string key = intToString(weekNum)+"_"+time;
			iter = TODOWResult.find(key);
			if(iter!=TODOWResult.end()){
				Data_TODOWMerge *obj_tw = &iter->second;
				map<int, int>::iterator c_iter = obj_tw->ClusterCount.find(clusterResult);
				if(c_iter!=obj_tw->ClusterCount.end()){
					c_iter->second++;
				}else{
					obj_tw->ClusterCount.insert(make_pair(clusterResult, 1));
				}				
			}else{
				Data_TODOWMerge obj_tw;
				obj_tw.ClusterCount.insert(make_pair(clusterResult, 1));
				TODOWResult.insert(make_pair(key, obj_tw));
			}
			begtime = moveMinutes(begtime, MinPeriod);
		}		
	}	
	MergeClusterData.clear();
		
	map<int, Data_DOWPeriod> tempPerResult;	map<int, Data_DOWPeriod>::iterator t_i; 
	for(iter=TODOWResult.begin();iter!=TODOWResult.end();iter++){		
		Data_TODOWMerge *obj_tw = &iter->second;
		obj_tw->computeProbAndCluNo();		
		vector<string> wt = split(iter->first, "_");
		int weekNum = atoi(wt[0].c_str()); string time = wt[1];		
		
		t_i = tempPerResult.find(weekNum);
		if(t_i!=tempPerResult.end()){
			Data_DOWPeriod *dowObj = &t_i->second;
			Data_TODPeriod *lastTODr = &dowObj->TOD_PerResult[dowObj->TOD_PerResult.size()-1];
			if(obj_tw->TODOWCCluNo==lastTODr->clusterResult){				
				lastTODr->CluNoProb = lastTODr->CluNoProb + obj_tw->CluNoProb;				
			}else{							
				lastTODr->endtime = time;
				lastTODr->computePeriodCount(MinPeriod);
				double temp = 0; if(lastTODr->CluNoProb>0 && lastTODr->periodCount>0) temp = lastTODr->CluNoProb/lastTODr->periodCount;
				lastTODr->CluNoProb = temp;
				//if(weekNum==6) cout<<lastTODr->begtime<<" , "<<lastTODr->endtime<<" , "<<lastTODr->CluNoProb<<" , "<<lastTODr->periodCount<<endl;				
				
				//insert new period
				Data_TODPeriod todObj;
				todObj.begtime = time;	todObj.CluNoProb = obj_tw->CluNoProb;
				todObj.clusterResult = obj_tw->TODOWCCluNo;							
				dowObj->TOD_PerResult.push_back(todObj);
			}
		}else{				
			//insert new period of weekNum
			Data_TODPeriod todObj;
			todObj.begtime = time;	todObj.CluNoProb = obj_tw->CluNoProb;
			todObj.clusterResult = obj_tw->TODOWCCluNo;	
			Data_DOWPeriod dowObj;
			dowObj.TOD_PerResult.push_back(todObj);			
			tempPerResult.insert(make_pair(weekNum, dowObj));
		}	
	}
	TODOWResult.clear();
	
	map<int, Data_DOWPeriod>::iterator dp_i;
	for(dp_i=tempPerResult.begin();dp_i!=tempPerResult.end();dp_i++){
		Data_DOWPeriod *dowObj = &dp_i->second;		
		
		/*****handle last one data*****/
		Data_TODPeriod *l_todObj = &dowObj->TOD_PerResult[dowObj->TOD_PerResult.size()-1];
		l_todObj->endtime = "23:55:00";	l_todObj->computePeriodCount(MinPeriod);
		l_todObj->periodCount = l_todObj->periodCount + 1;
		l_todObj->endtime = "00:00:00";
		double temp = 0; if(l_todObj->CluNoProb>0 && l_todObj->periodCount>0) temp = l_todObj->CluNoProb/l_todObj->periodCount;
		l_todObj->CluNoProb = temp;
		/*****handle last one data*****/	
		
		/*****check 15 minutes limit*****/	
		Data_DOWPeriod newdowObj;			
		for(unsigned int i=0;i<dowObj->TOD_PerResult.size();i++){		
			Data_TODPeriod todObj = dowObj->TOD_PerResult[i];
			if(i==0){
				newdowObj.TOD_PerResult.push_back(todObj);
			}else{
				Data_TODPeriod *lasttodObj = &newdowObj.TOD_PerResult[newdowObj.TOD_PerResult.size()-1];
				if((todObj.periodCount<periodLenLimit)||(lasttodObj->periodCount<periodLenLimit)){
					double newProb = 0;
					if(lasttodObj->periodCount >= todObj.periodCount){
						newProb = lasttodObj->periodCount*lasttodObj->CluNoProb;
					}else{
						newProb = todObj.periodCount*todObj.CluNoProb;
						lasttodObj->clusterResult = todObj.clusterResult;
					}
					lasttodObj->endtime = todObj.endtime;
					lasttodObj->periodCount = lasttodObj->periodCount + todObj.periodCount;
					lasttodObj->CluNoProb = newProb/lasttodObj->periodCount;					
				}else{
					newdowObj.TOD_PerResult.push_back(todObj);					
				}
			}			
			/*cout<<dp_i->first<<" , "<<todObj->begtime<<" , "<<todObj->endtime<<" , "<<todObj->clusterResult<<" , "<<todObj->periodCount<<" , ";
			cout<<todObj->CluNoProb<<endl;*/							
		}
		TODOW_PerResult.insert(make_pair(dp_i->first, newdowObj));
	}	
	OutputMergeResultByWeek(ProjectFolder+"ClusterResult/");
}

void ComputeCenter::OutputMergeResultByWeek(string folder){
	map<int, Data_DOWPeriod>::iterator iter;
	for(iter=TODOW_PerResult.begin();iter!=TODOW_PerResult.end();iter++){
		int weekNum = iter->first;				
		string weekData = folder+"WeekDay_"+intToString(weekNum)+".csv";
		char * week = new char[weekData.length()+1];
		strcpy(week, weekData.c_str());	
		fstream f_week; f_week.open(week, ios::out);
		if(!f_week){
			cout<<"Fail to open output data file: "<<week<<endl;
		}
		
		/*****insert Column Name*****/
		f_week<<"WeekNumber,BeginTime,EndTime,periodLength(Min),Probability,ClusterNo,SortClusNo,";
		for(unsigned int d=0;d<dirVertor.size();d++){
			if(d==(dirVertor.size()-1)){
				f_week<<"Center_"<<dirVertor[d];
			}else{
				f_week<<"Center_"<<dirVertor[d]<<",";
			}
		} 
		f_week<<endl;		
			
		/*****insert Column Name*****/		
		Data_DOWPeriod *dowObj = &iter->second;
		for(unsigned int i=0;i<dowObj->TOD_PerResult.size();i++){
			Data_TODPeriod *todObj = &dowObj->TOD_PerResult[i];
			int cluR = todObj->clusterResult;			
			f_week<<weekNum<<","<<todObj->begtime<<","<<todObj->endtime<<","<<todObj->periodCount*MinPeriod<<","<<todObj->CluNoProb<<","<<cluR<<",";			
			map<int, Data_KMeansMeans>::iterator km_i = KMeansMeans.find(cluR);
			Data_KMeansMeans *k_obj = &km_i->second;
			f_week<<k_obj->sortCluNumber;
			for(unsigned int j=0;j<k_obj->means.size();j++){				
				f_week<<","<<k_obj->means[j];				
			}
			f_week<<endl;			
		}
		f_week.close();		
	}		
}

void ComputeCenter::OutPutProcessResult(string content){
	string logfile = ProjectFolder+"log";
	fstream f_logfile; f_logfile.open(logfile.c_str(), ios::out);
	f_logfile<<content<<endl;
	f_logfile.close();	
}

//tool box
vector<string> ComputeCenter::split(string str, string pattern){
	size_t pos;
	vector<string> result;
	str += pattern;
	int size = str.size();
	for (int i = 0; i < size; i++){
		pos = str.find(pattern, i);
		if (pos < size){
			string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

string ComputeCenter::moveMinutes(string basepoint, int step){
	struct tm tm;
	strptime(basepoint.c_str(), "%Y/%m/%d %H:%M:%S", &tm);
	tm.tm_min += step;
    mktime(&tm);
	
	string result = intToString(1900+tm.tm_year)+"/"+intToString(tm.tm_mon+1)+"/"+intToString(tm.tm_mday)+" ";
	result = result+convertSpacePadded(tm.tm_hour)+":"+convertSpacePadded(tm.tm_min)+":"+convertSpacePadded(tm.tm_sec);	
	//cout<<result<<endl;
	return result;
}

string ComputeCenter::moveMinutesOnlyMin(string basepoint, int step){
	struct tm tm;
	strptime(basepoint.c_str(), "%H:%M:%S", &tm);
	tm.tm_min += step;
    mktime(&tm);
	
	string result = convertSpacePadded(tm.tm_hour)+":"+convertSpacePadded(tm.tm_min)+":"+convertSpacePadded(tm.tm_sec);	
	//cout<<result<<endl;
	return result;
}

string ComputeCenter::intToString(int integer){
    stringstream ss;
    ss<<integer;
    return ss.str();
}

string ComputeCenter::convertSpacePadded(int integer){	
	string result = intToString(integer);
	if(integer<10) result = "0"+result;
	return result;
}








