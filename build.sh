#!/bin/bash

SDK="SaleaeAnalyzerSDK-1.1.23"
outputDir="out"
outputName="libSDIOAnalyzer"
extension="dylib"

mkdir -p $outputDir

g++ -Wall -dynamiclib -undefined suppress -flat_namespace -I . -I $SDK/include -I $SDK/source -L $SDK/lib -l Analyzer -o $outputDir/$outputName.$extension source/*.cpp
