import numpy as np 
from sklearn.mixture import GaussianMixture
from sklearn.cluster import KMeans


def clusterByGMM(data, cluNum):	
	gmm = GaussianMixture(n_components=cluNum, covariance_type='spherical')
	gmm.fit(data)
	result = gmm.predict(data).tolist()
	gmm_means = gmm.means_.tolist()
	gmm_covars = gmm.covariances_.tolist()
	gmm_weights = gmm.weights_[0]		
	return result, gmm_means, gmm_covars, gmm_weights

def clusterByKmeans(data, cluNum):
	kmeans = KMeans(n_clusters=cluNum, random_state=0).fit(data)
	result = kmeans.labels_.tolist()
	kmeans_means = kmeans.cluster_centers_.tolist();	
	return result, kmeans_means
	
if __name__ == '__main__':
	'''
	data = np.array([[1, 2, 3, 2], [1, 4, 3, 2], [1, 0, 3, 2], [4, 2, 3, 2], [2, 4, 3, 2], [4, 0, 3, 2]])
	clusterByKmeans(data, 2)
	'''	
	
	
	
	
	