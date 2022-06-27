mkdir -p build
cd build/
mkdir -p debug
mkdir -p release
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../
cd ../release
cmake -DCMAKE_BUILD_TYPE=Release ../../
cd ..
cd ..
