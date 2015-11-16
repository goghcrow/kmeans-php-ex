<?php
$data = file_get_contents("orderdata.json");
$data = json_decode($data, true);
$points = [];
foreach ($data as $index => $value) {
	$points[$index] = [floatval($value["lng"]), floatval($value["lat"])];
}

/**
 * $points = [
 * 	[dim1, dim2, ...],[dim1, dim2, ...],[dim1, dim2, ...]
 * ];
 */

$start = microtime(true);
$dimension = 2;
$clusters = 200;
$kret = kmeans($points, $dimension, $clusters);
if($kret === false) {
	exit(1);
}


$ret = [];
foreach ($kret["pointInCluster"] as $pointIndex => $clusterIndex) {
	if(!isset($ret[$clusterIndex])) {
		$ret[$clusterIndex] = [
			"centroid"	=> $kret["clusters"][$clusterIndex],
			"points"	=> [],
		];
	}
	$ret[$clusterIndex]["points"][] = $data[$pointIndex];
}
$ret = array_values($ret);
// ksort($ret, SORT_NUMERIC);
echo microtime(true) - $start . PHP_EOL;
echo "iterCount : ", $kret["iterCount"] . PHP_EOL;

foreach ($ret as $index => $cluster) {
	echo "cluster $index has ", count($cluster["points"]), " points" . PHP_EOL;
}

file_put_contents("ret.txt", print_r($ret, true));
