#include "SAPI.h"
#include "zend_API.h"
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_variables.h"
#include "php_kmeans.h"

/*
ZEND_DECLARE_MODULE_GLOBALS(kmeans);
*/

inline static
prec_t calcDist(prec_t *point1, prec_t *point2, size_t dimension)
{
  prec_t distance = 0;
  size_t i;

  for (i = 0; i < dimension; i++)
  {
    distance += (point1[i] - point2[i]) * (point1[i] - point2[i]);
  }

  /* 仅用来比较大小，没必要开方 return sqrt(distance); */
  return distance;
}

inline static
prec_t calcNorm(prec_t *point, size_t dimension)
{
  prec_t sum = 0;
  size_t i;
  for (i = 0; i < dimension; i++)
  {
    sum += point[i] * point[i];
  }
  return sum;
}

inline static
size_t eq(prec_t *point, prec_t *cluster, size_t dimension)
{
  size_t i;
  for (i = 0; i < dimension; i++)
  {
    if(point[i] != cluster[i])
    {
      return 0;
    }
  }
  return 1;
}

inline static
void cp(prec_t *dest, prec_t *src, size_t dimension)
{
  // memcpy(dest, src, dimension);
  while (dimension--)
  {
    dest[dimension] = src[dimension];
  }
}

inline static
size_t findMaxIndex(prec_t *array, size_t n)
{
  size_t i, max_index = 0;
  prec_t max_value = PREC_MIN;

  for (i = 0; i < n; i++)
  {
    if (array[i] > max_value)
    {
      max_value = array[i];
      max_index = i;
    }
  }
  return max_index;
}

inline static
size_t findClosestCluster(prec_t *point,
              prec_t **clusters,
              size_t dimension,
              size_t num_clusters)
{
  size_t min_index, i;
  prec_t dist, min_dist;

  min_index = 0;
  min_dist = calcDist(point, clusters[0], dimension);

  for (i = 0; i < num_clusters; i++)
  {
    dist = calcDist(point, clusters[i], dimension);
    if (dist < min_dist)
    {
      min_dist = dist;
      min_index = i;
    }
  }
  return min_index;
}

inline
void getClusterRand(prec_t **points,
            size_t num_points,
            size_t dimension,
            size_t num_clusters,
            PARAM_OUT prec_t **clusters)
{
  size_t i, j;
  for (i = 0; i < num_clusters; i++)
  {
    for (j = 0; j < dimension; j++)
    {
      clusters[i][j] = points[i][j];
    }
  }
}


/* 找到尽可能分散的点作为初始cluster */
inline
void getClusterKKZ(prec_t **points,
             size_t num_points,
             size_t dimension,
             size_t num_clusters,
             PARAM_OUT prec_t **clusters)
{
  prec_t *norm_vector = NULL;
  prec_t *distance_vector = NULL;
  prec_t min_distance = PREC_MAX, distance;

  size_t i, j;
  size_t centroids_index = 0, flag = 0;

#define FOR_POINTS for (i = 0; i < num_points; i++)
#define FOR_CLUSTERS for (i = 0; i < num_clusters; i++)

  norm_vector = (prec_t*)emalloc(num_points * sizeof(prec_t));
  distance_vector = (prec_t*)emalloc(num_points * sizeof(prec_t));

  FOR_POINTS
  {
    norm_vector[i] = calcNorm(points[i], dimension);
  }
  cp(clusters[centroids_index], points[findMaxIndex(norm_vector, num_points)], dimension);
  centroids_index++;

  FOR_POINTS
  {
    distance_vector[i] = calcDist(points[i], clusters[0], dimension);
  }
  cp(clusters[centroids_index], points[findMaxIndex(distance_vector, num_points)], dimension);
  centroids_index++;

  if (num_clusters > 2)
  {
    do
    {
      FOR_POINTS
      {
        distance_vector[i] = 0;
        for (j = 0; j < centroids_index; j++)
        {
          if(eq(points[i], clusters[j], dimension) == 1)
          {
            flag = 1;
            continue;
          }
          distance = calcDist(points[i], clusters[j], dimension);

          if(distance < min_distance)
          {
            min_distance = distance;
          }
        }

        if (flag == 1)
        {
          distance_vector[i] = PREC_MIN;
          flag = 0;
        }
        else
        {
          distance_vector[i] = min_distance;
        }
      }

      cp(clusters[centroids_index], points[findMaxIndex(distance_vector, num_points)], dimension);

      centroids_index++;

    } while (centroids_index < num_clusters);
  }

  efree(norm_vector);
  efree(distance_vector);
}


inline
size_t kmeans(prec_t **points,
        prec_t **clusters,
        int *pointInCluster,
        size_t num_points,
        size_t dimension,
        size_t num_clusters,
        prec_t threshold,
        size_t max_iter_count
         )
{
  size_t i, j, closest_index;

  prec_t clusterUpdatePercentage;
  size_t centroidChangCount;
  size_t iterCount = 0;

  prec_t **tmpNewClusters = NULL;
  size_t *tmpNewClusterSize;

  ecalloc2D(tmpNewClusters, num_clusters, dimension, prec_t);

  tmpNewClusterSize = (size_t*)ecalloc(num_clusters, sizeof(size_t));


  for (i = 0; i < num_points; i++)
  {
    pointInCluster[i] = -1;
  }


  while(1)
  {
    clusterUpdatePercentage = 0.0;
    centroidChangCount = 0;

    for (i = 0; i < num_points; i++)
    {
      closest_index = findClosestCluster(points[i], clusters, dimension, num_clusters);
      if (closest_index != pointInCluster[i])
      {
        clusterUpdatePercentage += 1.0;
        pointInCluster[i] = closest_index;
      }

      tmpNewClusterSize[closest_index]++;

      for (j = 0; j < dimension; j++)
      {
        tmpNewClusters[closest_index][j] += points[i][j];
      }
    }


    for (i = 0; i < num_clusters; i++)
    {
      for (j = 0; j < dimension; j++)
      {
        if (tmpNewClusterSize[i] > 1)
        {
          if (clusters[i][j] != (tmpNewClusters[i][j] / tmpNewClusterSize[i]))
          {
            centroidChangCount++;
          }
          clusters[i][j] = tmpNewClusters[i][j] / tmpNewClusterSize[i];
        }
        else
        {
          if (clusters[i][j] != tmpNewClusters[i][j])
          {
            centroidChangCount++;
          }
          clusters[i][j] = tmpNewClusters[i][j];
        }

        tmpNewClusters[i][j] = 0.0;
      }

      tmpNewClusterSize[i] = 0;
    }

    clusterUpdatePercentage /= num_points;

    if(centroidChangCount == 0 ||
      clusterUpdatePercentage <= threshold ||
      ++iterCount >= max_iter_count)
    {
      break;
    }
  }

  efree(tmpNewClusterSize);
  efree2D(tmpNewClusters);

  return iterCount + 1;
}



/* {{{ proto int kmeans(array $points, $numClusters [, $threshold = 0.0, $maxIterCount = 500])
        */
ZEND_FUNCTION(kmeans)
{
	size_t dimension;
	long num_clusters;
	double threshold = 0.0;
	long maxIterCount = 500;

	size_t num_points;
	size_t i, j, hash_index;

	size_t iterCount;
	prec_t **points = NULL;
	prec_t **clusters = NULL;
	int *pointInCluster = NULL;

	zval *clustersRetval = NULL;
	zval *pointInClusterRetval = NULL;

	HashTable *ht_points;
	HashPosition pointer;
	zval **point;

	array_init(return_value);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"hll|dl", &ht_points, &dimension, &num_clusters, &threshold, &maxIterCount) == FAILURE)
	{
		RETURN_NULL();
	}

	num_points = zend_hash_num_elements(ht_points);
	if(num_points < num_clusters || num_clusters < 1 || dimension < 1)
	{
		RETURN_FALSE;
	}

	// malloc2D(points, num_points, dimension, prec_t);
	points = (prec_t **)emalloc(num_points * sizeof(prec_t *));
	points[0] = (prec_t *)emalloc(num_points * dimension * sizeof(prec_t));
	for (i = 1; i < num_points; i++)
	{														
		points[i] = points[i-1] + dimension;
	}


	hash_index = 0;
	for (zend_hash_internal_pointer_reset_ex(ht_points, &pointer);
			zend_hash_get_current_data_ex(ht_points, (void**)&point, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(ht_points, &pointer)) {

		zval **dimval, **zkeyval = NULL;
		HashTable *ht_point;

		if (Z_TYPE_PP(point) != IS_ARRAY) {
			// php_error_docref(NULL TSRMLS_CC, E_WARNING, "points must be one dimension array");
			RETURN_FALSE;
		}
		ht_point = Z_ARRVAL_PP(point);
		
		for (i = 0; i < dimension; i++)
		{
			if ((zend_hash_index_find(ht_point, i, (void**)&dimval) == FAILURE) ||
				(Z_TYPE_PP(dimval) != IS_LONG &&
				Z_TYPE_PP(dimval) != IS_DOUBLE))
			{
				// php_error_docref(NULL TSRMLS_CC, E_WARNING, "");
				RETURN_FALSE;
			}

			convert_to_double(*dimval);
			points[hash_index][i] = Z_DVAL_PP(dimval);
			// php_printf("%.3f\n", points[hash_index][i]);
		}

		hash_index++;
	}


	emalloc2D(clusters, num_clusters, dimension, prec_t);
	// getClusterRand(points, num_points, dimension, num_clusters, clusters);
	getClusterKKZ(points, num_points, dimension, num_clusters, clusters);

	pointInCluster = (int*)emalloc(num_points * sizeof(int));

	iterCount = kmeans(points, clusters, pointInCluster, num_points, dimension, num_clusters, 0.0, 500);
	// php_printf("iterator %d\n", (int)iterCount);

	MAKE_STD_ZVAL(clustersRetval);
	array_init(clustersRetval);
	
	MAKE_STD_ZVAL(pointInClusterRetval);
	array_init(pointInClusterRetval);

	for (i = 0; i < num_points; i++)
	{
		// php_printf("data point %d is in cluster %d\n", (int)i, pointInCluster[i]);
		add_index_double(pointInClusterRetval, i, pointInCluster[i]);
	}

	for (i = 0; i < num_clusters; i++)
	{
		zval *centroidVal;
		MAKE_STD_ZVAL(centroidVal);
		array_init(centroidVal);

		for (j = 0; j < dimension; j++)
		{
			add_index_double(centroidVal, j, clusters[i][j]);
		}
		add_index_zval(clustersRetval, i, centroidVal);
	}
	

	add_assoc_long(return_value, "iterCount", iterCount);
	add_assoc_zval(return_value, "clusters", clustersRetval);
	add_assoc_zval(return_value, "pointInCluster", pointInClusterRetval);


	efree2D(points);
	efree(pointInCluster);
	efree2D(clusters);
	



	return;
}
/* }}} */



/* static */ zend_function_entry kmeans_functions[] = {
  ZEND_FE(kmeans, NULL)
  ZEND_FE_END /* {NULL, NULL, NULL} */
};


/* {{{ kmeans_module_entry
 */
zend_module_entry kmeans_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
     STANDARD_MODULE_HEADER,
#endif
    PHP_KMEANS_MODULE_NAME, /* 这个地方是扩展名称，往往我们会在这个地方使用一个宏 */
    kmeans_functions, /* Functions */
    NULL, /* MINIT */
    NULL, /* MSHUTDOWN */
    NULL, /* RINIT */
    NULL, /* RSHUTDOWN */
    NULL, /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_KMEANS_VERSION, /* 这个地方是我们扩展的版本 */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */


#ifdef COMPILE_DL_KMEANS
ZEND_GET_MODULE(kmeans)
#endif