#!/bin/bash
# requires jq
# $> sudo apt-get install jq

cwd=$(pwd)

source regressionTestDef.sh

for app in ${applications[*]}
do
    cd $PISA_EXAMPLES/compile/$app
    make clean
    echo "TESTING $app"
    make pisa &> /dev/null
    if [ $? -ne 0 ]; then
        echo "COMPILATION ERROR FOR $app"
        exit 1
    fi
    make install &> /dev/null
    if [ $? -ne 0 ]; then
        echo "INSTALLATION ERROR FOR $app"
        exit 1
    fi

    cd $PISA_EXAMPLES/profile/$app

    rm -f log
    env PISAFileName=output.cnls make pisa &> /dev/null
    if [ $? -ne 0 ]; then
        echo "PISA EXECUTION ERROR FOR $app"
        exit 1
    fi
    sed -i '/\"time\":/d' output.cnls

    diff output.cnls $PISA_EXAMPLES/references/$app.cnls > differences.cnls
        
    DIFF=$(cat differences.cnls)
    if [ "$DIFF" != "" ] 
    then
        echo ""
        echo "POSSIBLE PROBLEMS IN CNLS ANALYSIS:"
        echo $DIFF
        echo "PLEASE DOUBLE CHECK $(pwd)/differences.cnls"
        echo "Or use a GUI to compare the output files: $> kompare $(pwd)/output.cnls $PISA_EXAMPLES/references/$app.cnls"
    else
        echo "Coupled profile for $app is FINE"
    fi


    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
    echo ""
done

cd $cwd
