#!/bin/bash

SDK="SaleaeAnalyzerSDK-1.1.23"
outputDir="out"
outputName="libSDIOAnalyzer"
extension="dylib"

mkdir -p $outputDir

g++ -I . -I $SDK/include -I $SDK/source -L $SDK/lib -l Analyzer -dynamiclib -undefined suppress -flat_namespace -o $outputDir/$outputName.$extension source/*.cpp
