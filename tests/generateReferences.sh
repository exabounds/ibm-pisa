#!/bin/bash
# requires jq
# $> sudo apt-get install jq

source regressionTestDef.sh

cwd=$(pwd)

for app in ${applications[*]}
do
    echo Generating reference for $app
    cd $PISA_EXAMPLES/compile/$app
    make clean
    make pisa
    make install

    cd $PISA_EXAMPLES/profile/$app

    env PISAFileName=$app.cnls make pisa
    cp $app.cnls $PISA_EXAMPLES/references/$app.cnls
    sed -i '/\"time\":/d' $PISA_EXAMPLES/references/$app.cnls
done

cd $cwd
