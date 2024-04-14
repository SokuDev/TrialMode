rm -rf build
mkdir build
cd build || exit
make -C ../cmake-build-release* TrialMode || exit
cp ../cmake-build-release*/TrialMode.dll . || exit
make -C ../cmake-build-debug* TrialMode || exit
cp ../cmake-build-debug*/TrialMode.dll ./TrialModeDebug.dll || exit
cp ../cmake-build-debug*/TrialMode.pdb ./TrialModeDebug.pdb || exit
cp -r ../packs . || exit
find packs -name "score.dat" -delete -print
find packs -name "*.backup" -delete -print
zip -r "../TrialMode_$(cat ../src/version.h | cut -d '"' -f 2 | tr ' ' '_').zip" *
cd ..
rm -rf build