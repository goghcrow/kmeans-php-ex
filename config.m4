PHP_ARG_ENABLE(
  kmeans,
    [Whether to enable the "kmeans" extension],
    [  enable-kmeans        Enable "kmeans" extension support]
)

if test $PHP_KMEANS != "no"; then
    PHP_SUBST(KMEANS_SHARED_LIBADD)
    PHP_NEW_EXTENSION(kmeans, kmeans.c, $ext_shared)
fi
