//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

// StE
// © Shlomi Steinberg, 2015-2016

#pragma once

#include <atomic>
#include <vector>
#include <type_traits>

namespace StE {

template <typename T, int N = 3>
class concurrent_pointer_recycler {
private:
	std::array<std::atomic<T*>, N> pointers;

	static_assert(std::is_move_assignable<T>::value, "T must be move-assignable");

public:
	~concurrent_pointer_recycler() {
		for (auto &slot : pointers) {
			T* ptr = slot.load(std::memory_order_acquire);
			if (ptr)
				delete ptr;
		}
	}

	void release(T *ptr) {
		T* null = 0;
		for (auto &slot : pointers)
			if (slot.compare_exchange_strong(null, ptr, std::memory_order_relaxed)) {
				return;
			}
		delete ptr;
	}

	template <typename ... Ts>
	T* claim(Ts&&... args) {
		T* ptr;
		for (auto &slot : pointers) {
			ptr = slot.load(std::memory_order_relaxed);
			if (ptr && slot.compare_exchange_strong(ptr, 0, std::memory_order_relaxed)) {
				*ptr = T(std::forward<Ts>(args)...);
				return ptr;
			}
		}

		return new T(std::forward<Ts>(args)...);
	}
};

}
