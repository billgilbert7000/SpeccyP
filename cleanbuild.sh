cd build

rm -r *
cmake -DPICO_BOARD=pico ..
cmake --build . --parallel
cp *.uf2 ../bin

rm -r *
cmake -DPICO_BOARD=pico2 ..
cmake --build . --parallel
cp *.uf2 ../bin