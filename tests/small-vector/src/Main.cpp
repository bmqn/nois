#include <nois/util/NoisSmallVector.hpp>

#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <string>
#include <vector>

struct Trackable
{
	static inline int constructions = 0;
	static inline int destructions = 0;
	int value;

	Trackable(int v = 0)
		: value(v)
	{
		++constructions;
	}

	Trackable(const Trackable &other)
		: value(other.value)
	{
		++constructions;
	}

	Trackable(Trackable &&other) noexcept
		: value(other.value)
	{
		++constructions;
		other.value = -1;
	}

	~Trackable()
	{
		++destructions;
	}

	Trackable &operator=(const Trackable &other)
	{
		value = other.value;
		return *this;
	}

	Trackable &operator=(Trackable &&other) noexcept
	{
		value = other.value;
		other.value = -1;
		return *this;
	}

	operator int() const
	{
		return value;
	}
};

template <typename T, std::size_t N>
void test_smallvector_basic()
{
	nois::SmallVector<T, N> vec;

	// Test empty and size
	assert(vec.empty());
	assert(vec.size() == 0);

	// Push back inline
	for (int i = 0; i < static_cast<int>(N); ++i)
	{
		vec.push_back(i);
		assert(vec.back() == i);
		assert(vec.size() == i + 1);
	}
	assert(!vec.empty());

	// Push back beyond inline → fallback
	vec.push_back(N);
	assert(vec.size() == N + 1);
	assert(vec.back() == N);

	// Test operator[]
	for (size_t i = 0; i < vec.size(); ++i)
		assert(vec[i] == static_cast<int>(i));

	// Test front/back
	assert(vec.front() == 0);
	assert(vec.back() == static_cast<int>(N));

	// Test pop_back
	vec.pop_back();
	assert(vec.size() == N);
	assert(vec.back() == static_cast<int>(N - 1));

	// Resize smaller
	vec.resize(N - 2);
	assert(vec.size() == N - 2);
	for (size_t i = 0; i < vec.size(); ++i)
		assert(vec[i] == static_cast<int>(i));

	// Resize larger inline
	vec.resize(N);
	assert(vec.size() == N);
	for (size_t i = 0; i < N - 2; ++i)
		assert(vec[i] == static_cast<int>(i));

	// Resize into fallback
	vec.resize(N + 3);
	assert(vec.size() == N + 3);
	for (size_t i = 0; i < N - 2; ++i)
		assert(vec[i] == static_cast<int>(i));
	for (size_t i = N; i < N + 3; ++i)
		assert(vec[i] == 0); // default initialized

	// Clear
	vec.clear();
	assert(vec.empty());
}

template <typename T, std::size_t N>
void test_smallvector_insert_erase()
{
	nois::SmallVector<T, N> vec;
	for (int i = 0; i < static_cast<int>(N + 2); ++i)
		vec.push_back(i);

	// Insert in the middle
	auto it = vec.begin() + 2;
	vec.insert(it, 42);
	assert(vec[2] == 42);
	assert(vec.size() == N + 3);

	// Erase the element we just inserted
	vec.erase(vec.begin() + 2);
	assert(vec[2] == 2);
	assert(vec.size() == N + 2);

	// Insert at the end
	vec.insert(vec.end(), 99);
	assert(vec.back() == 99);

	// Insert at the beginning
	vec.insert(vec.begin(), -1);
	assert(vec.front() == -1);
}

template <typename T, std::size_t N>
void test_smallvector_copy_move()
{
	nois::SmallVector<T, N> vec;
	for (int i = 0; i < static_cast<int>(N + 1); ++i)
		vec.push_back(i);

	// Copy
	nois::SmallVector<T, N> copy = vec;
	assert(copy.size() == vec.size());
	for (size_t i = 0; i < copy.size(); ++i)
		assert(copy[i] == vec[i]);

	// Move
	nois::SmallVector<T, N> moved = std::move(vec);
	assert(moved.size() == copy.size());
	for (size_t i = 0; i < moved.size(); ++i)
		assert(moved[i] == copy[i]);
	assert(vec.size() == 0);
}

template <typename T, std::size_t N>
void test_smallvector_iterators()
{
	nois::SmallVector<T, N> vec;
	for (int i = 0; i < static_cast<int>(N + 2); ++i)
		vec.push_back(i);

	int sum = 0;
	for (auto &v : vec)
	{
		sum += v;
	}
	int expected = 0;
	for (int i = 0; i < static_cast<int>(N + 2); ++i)
		expected += i;
	assert(sum == expected);

	// Random access
	auto it = vec.begin();
	it += 3;
	assert(*it == 3);
	it -= 2;
	assert(*it == 1);
}

template <typename Vec>
void test_predictable_benchmark(const std::string &name, size_t iterations = 100000, size_t max_size = 32)
{
	using Clock = std::chrono::high_resolution_clock;

	Clock::time_point start = Clock::now();

	size_t counter = 0;
	for (size_t i = 0; i < iterations; ++i)
	{
		Vec vec;
		vec.reserve(max_size);
		for (int j = 0; j < max_size; ++j)
		{
			if (j == 16)
			{
				vec.insert(vec.begin(), 100);
			}
			else
			{
				vec.push_back(j);
			}
		}
		assert(vec.size() == max_size);
		for (int j = 0; j < 16; ++j)
		{
			vec.erase(vec.begin() + 16);
		}
		assert(vec.size() == 16);
		for (int j = 0; j < vec.size(); ++j)
		{
			counter += (vec[j] * (vec[j] > 0 ? (typename Vec::value_type(vec[2])) : (typename Vec::value_type(0))));
		}
		assert(counter == (100 + 15 * (15 - 1) / 2) * (i + 1));
	}

	Clock::time_point end = Clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << name << ": " << duration << " µs" << ", counter: " << counter << std::endl;
}

template <typename Vec>
void test_random_benchmark(const std::string &name, size_t iterations = 100000, size_t max_size = 32)
{
	using Clock = std::chrono::high_resolution_clock;

	std::mt19937 rng(12345);
	std::uniform_int_distribution<int> dist_value(0, 100);
	std::uniform_real_distribution<float> dist_op(0.0f, 1.0f);

	Clock::time_point start = Clock::now();

	Vec vec;
	vec.reserve(max_size);
	size_t counter = 0;
	for (size_t it = 0; it < iterations; ++it)
	{
		float op = dist_op(rng);
		if ((op < 0.5f && !vec.empty()) || vec.size() == max_size)
		{
			size_t pos = std::uniform_int_distribution<size_t>(0, vec.size() - 1)(rng);
			vec.erase(vec.begin() + pos);
		}
		else
		{
			size_t pos = std::uniform_int_distribution<size_t>(0, vec.size())(rng);
			vec.insert(vec.begin() + pos, dist_value(rng));
		}
		for (size_t j = 0; j < vec.size(); ++j)
		{
			counter += vec[j];
		}
	}

	Clock::time_point end = Clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << name << ": " << duration << " µs, counter: " << counter << std::endl;
}

int main()
{
	Trackable::constructions = 0;
	Trackable::destructions = 0;

	std::cout << "Testing nois::SmallVector<int, 4>..." << std::endl;
	test_smallvector_basic<int, 4>();
	test_smallvector_insert_erase<int, 4>();
	test_smallvector_copy_move<int, 4>();
	test_smallvector_iterators<int, 4>();

	std::cout << "Testing nois::SmallVector<Trackable, 4>..." << std::endl;
	test_smallvector_basic<Trackable, 4>();
	test_smallvector_insert_erase<Trackable, 4>();
	test_smallvector_copy_move<Trackable, 4>();
	test_smallvector_iterators<Trackable, 4>();

	std::cout << "Testing performance (predictable)..." << std::endl;
	test_predictable_benchmark<nois::SmallVector<int, 32>>("SmallVector<int, 32>");
	test_predictable_benchmark<std::vector<int>>("std::vector<int>");
	test_predictable_benchmark<nois::SmallVector<Trackable, 32>>("SmallVector<Trackable, 32>");
	test_predictable_benchmark<std::vector<Trackable>>("std::vector<Trackable>");

	std::cout << "Testing performance (random)..." << std::endl;
	test_random_benchmark<nois::SmallVector<int, 32>>("SmallVector<int, 32>");
	test_random_benchmark<std::vector<int>>("std::vector<int>");
	test_random_benchmark<nois::SmallVector<Trackable, 32>>("SmallVector<Trackable, 32>");
	test_random_benchmark<std::vector<Trackable>>("std::vector<Trackable>");

	assert(Trackable::constructions == Trackable::destructions);

	std::cout << "All tests passed!" << std::endl;

	return 0;
}
