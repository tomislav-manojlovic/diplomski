if [ ! -d build ]
then
	mkdir build
fi

for src_file in src/*.cpp; do
	echo "Compiling $src_file..."
	exe_file=$(basename $src_file .cpp)
	g++ -O3 -std=c++17 -march=native $src_file -o build/$exe_file
done

echo "Done."
