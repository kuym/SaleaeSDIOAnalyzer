#!/bin/bash

pushd .. > /dev/null

./build.sh
result=$?

popd > /dev/null

exit $result
