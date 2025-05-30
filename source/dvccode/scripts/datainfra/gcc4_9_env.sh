# GCC
export NEW_GCC_BIN=/usr/local/bin
export PATH=$NEW_GCC_BIN:$PATH
export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


export GCC_4_9_ROOT=/apps/gcc_versions/gcc-4_9_install
if [ -d $GCC_4_9_ROOT ] ; then
   export PATH=$GCC_4_9_ROOT/bin:$PATH;
   export LD_LIBRARY_PATH=$GCC_4_9_ROOT/lib64:$LD_LIBRARY_PATH ;
   # export LIBRARY_PATH=$GCC_4_9_ROOT/lib64:$LIBRARY_PATH ; # Not sure what this variable is. Sometimes it comes in handy
fi

# BOOST
export BOOST_4_9_PATH=/apps/boost/boost-install-4.9-new
if [ -d $BOOST_4_9_PATH/lib ] ; then
    export BOOST_LIB_INSTALL=$BOOST_4_9_PATH/lib
fi
if [ -d $BOOST_4_9_PATH/include ] ; then
    export BOOST_INCLUDE_BASE=$BOOST_4_9_PATH/include
fi
