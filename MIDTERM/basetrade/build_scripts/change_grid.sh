#!/bin/bash

set -e

if [ -z "${1}" ]
	then
		echo "No arguement supplied"
		exit 1
fi

echo "${1}"


jsonObj="[
		{
			\"name\":\"basetrade\",
			\"bucket\":\"dvc-artifactory\",
			\"key\":\"${1}\"
		}
]"

echo "${jsonObj}"

WORK_DIR=$(dirname $0)/..
GRID_TEMP_DIR=$(realpath $WORK_DIR)/gridTemp

if [ -d $GRID_TEMP_DIR ];
then
	rm -rf $GRID_TEMP_DIR
fi

mkdir $GRID_TEMP_DIR
git clone ssh://git@github.com/cvquant/grid.git $GRID_TEMP_DIR
cd $GRID_TEMP_DIR
git checkout master
echo "${jsonObj}" > worker_dependencies.json
git add .
git commit -m "Updating basetrade version"
git push origin master
git checkout prod
git merge master
git push origin prod
sudo rm -rf $GRID_TEMP_DIR
