#include <random>

void seedArray(uint8_t *arr, size_t len) {
	std::default_random_engine engine;
	std::uniform_int_distribution<int> distro;

	for(int i = 0; i < len; ++i) {
		arr[i] = distro(engine);
	}
}