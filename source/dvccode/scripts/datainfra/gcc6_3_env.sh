# GCC
export NEW_GCC_BIN=/usr/local/bin
export PATH=$NEW_GCC_BIN:$PATH
export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


export GCC_6_3_ROOT=/apps/gcc_versions/gcc-6_3_install
if [ -d $GCC_6_3_ROOT ] ; then
   export PATH=$GCC_6_3_ROOT/bin:$PATH;
   export LD_LIBRARY_PATH=$GCC_6_3_ROOT/lib64:$LD_LIBRARY_PATH ;
fi

# BOOST
if [ -d /apps/boost/boost-install-6.3/lib ] ; then
    export BOOST_LIB_INSTALL=/apps/boost/boost-install-6.3/lib
fi
if [ -d /apps/boost/boost-install-6.3/include ] ; then
    export BOOST_INCLUDE_BASE=/apps/boost/boost-install-6.3/include
fi
