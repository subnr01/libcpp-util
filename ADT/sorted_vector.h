//============================================================================
//                                  libcpp-util
//                   A simple odds-n-ends library for C++11
//
//         Licensed under modified BSD license. See LICENSE for details.
//============================================================================

#ifndef LIBCPP_SORTED_VECTOR_H
#define LIBCPP_SORTED_VECTOR_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

template <class Key, class Compare = std::less<Key>,
	  class Allocator = std::allocator<Key>>
class sorted_vector {
private:
	std::vector<Key, Allocator> storage;

protected:
	Compare comp;
	// FIXME: Figure out the deal with constness and standard algos for the
	// comparator
	std::reference_wrapper<Compare> comp_refwrap() {
		return std::ref(comp);
	}

public:
	typedef Key key_type;
	typedef Key value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef Compare key_compare;
	typedef Compare value_compare;
	typedef typename decltype(storage)::allocator_type allocator_type;
	typedef value_type &reference_type;
	typedef const value_type &const_reference_type;
	typedef typename std::allocator_traits<allocator_type>::pointer pointer;
	typedef typename std::allocator_traits<allocator_type>::const_pointer
	const_pointer;
	typedef typename decltype(storage)::iterator iterator;
// My libstdc++ version doesn't have correct insert/erase iterator decls
// (fixed in trunk). Use this ugly ifdef for now.
#ifndef CORRECT_CONST_ITERATOR_CONVENTIONS
	typedef typename decltype(storage)::iterator const_iterator;
#else
	typedef typename decltype(storage)::const_iterator const_iterator;
#endif
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
	bool equivalent(const Key &k1, const Key &k2) {
		return !comp(k1, k2) && !comp(k2, k1);
	}
	bool greater(const Key &k1, const Key &k2) { return comp(k2, k1); }

	// Checks positions to the left and right to see if ordering is correct
	bool good_hint(const_iterator hint, const value_type &k) {
		if (hint != begin() && !greater(k, *std::prev(hint)))
			return false;
		if (hint != end() && !comp(k, *hint))
			return false;
		return true;
	}
	void sort_unique(iterator first, iterator last) {
		std::sort(first, last, comp_refwrap());
		auto f = [this](const value_type &a, const value_type &b)
			     ->bool { return this->equivalent(a, b); };
		storage.erase(std::unique(first, last, f), end());
	}

public:
	// Constructors, assignment, destructor
	explicit sorted_vector(const Compare &c = Compare(),
			       const Allocator &a = Allocator())
		: storage(a), comp(c) {}
	template <class InputIt>
	sorted_vector(InputIt first, InputIt last, const Compare &c = Compare(),
		      const Allocator &a = Allocator())
		: storage(first, last, a), comp(c) {
		sort_unique(begin(), end());
	}
	sorted_vector(const sorted_vector<Key, Compare, Allocator> &x) =
	    default;
	sorted_vector(sorted_vector<Key, Compare, Allocator> &&x) = default;

	explicit sorted_vector(const Allocator &a) : storage(a) {}

	sorted_vector(const sorted_vector &x, const Allocator &a)
		: storage(x, a) {}
	sorted_vector(sorted_vector &&x, const Allocator &a)
		: storage(std::move(x), a) {}

	sorted_vector(std::initializer_list<value_type> ilist,
		      const Compare &c = Compare(),
		      const Allocator &a = Allocator())
		: storage(ilist, a), comp(c) {
		sort_unique(begin(), end());
	}

	sorted_vector &operator=(const sorted_vector &) = default;
	sorted_vector &operator=(sorted_vector &&) = default;
	sorted_vector &operator=(std::initializer_list<value_type> ilist) {
		storage.assign(ilist);
		sort_unique(begin(), end());
	}

	~sorted_vector() = default;

	// Iterators
	iterator begin() { return storage.begin(); }
	const_iterator begin() const { return storage.begin(); }
	const_iterator cbegin() const { return storage.cbegin(); }

	iterator end() { return storage.end(); }
	const_iterator end() const { return storage.end(); }
	const_iterator cend() const { return storage.cend(); }

	reverse_iterator rbegin() { return storage.rbegin(); }
	const_reverse_iterator rbegin() const { return storage.rbegin(); }
	const_reverse_iterator crbegin() const { return storage.crbegin(); }

	reverse_iterator rend() { return storage.rend(); }
	const_reverse_iterator rend() const { return storage.rend(); }
	const_reverse_iterator crend() const { return storage.crend(); }

	// Accessors for functions
	bool empty() const noexcept { return storage.empty(); }
	size_type size() const noexcept { return storage.size(); }
	size_type max_size() const noexcept { return storage.max_size(); }

	key_compare key_comp() const { return comp; }
	value_compare value_comp() const { return comp; }

	// Functions not in std::set but useful from a performance angle
	void reserve(size_type count) { storage.reserve(count); }
	size_type capacity() const { return storage.capacity(); }

	// Modifiers
	void clear() noexcept { storage.clear(); }

	iterator erase(const_iterator pos) { return storage.erase(pos); }

	iterator erase(const_iterator first, const_iterator last) {
		return storage.erase(first, last);
	}

	size_type erase(const key_type &key) {
		auto I = find(key);
		if (I != end()) {
			storage.erase(I);
			return 1;
		} else {
			return 0;
		}
	}

	void swap(sorted_vector &other) { storage.swap(other.storage); }

	// insert, emplace, emplace_hint
	std::pair<iterator, bool> insert(const value_type &value) {
		auto I = lower_bound(value);
		if (I != end() && equivalent(*I, value))
			return std::make_pair(I, false); // Got it already
		auto RetI = storage.insert(I, value);
		return std::make_pair(RetI, true);
	}
	std::pair<iterator, bool> insert(value_type &&value) {
		auto I = lower_bound(value);
		if (I != end() && equivalent(*I, value)) {
			return std::make_pair(I, false); // Got it already
		}
		auto RetI = storage.insert(I, std::move(value));
		return std::make_pair(RetI, true);
	}

	iterator insert(const_iterator hint, const value_type &value) {
		if (good_hint(hint, value))
			storage.insert(hint, value);
		else
			insert(value);
	}
	iterator insert(const_iterator hint, value_type &&value) {
		if (good_hint(hint, value))
			storage.insert(hint, std::move(value));
		else
			insert(std::move(value));
	}

	template <class InputIt> void insert(InputIt first, InputIt last) {
		// If we don't have any pre-existing elements, just sort and
		// unique.
		if (empty()) {
			storage.insert(begin(), first, last);
			sort_unique(begin(), end());
			return;
		}
		storage.reserve(std::distance(first, last) + storage.size());
		while (first != last)
			insert(*first++);
	}

	void insert(std::initializer_list<value_type> ilist) {
		insert(ilist.begin(), ilist.end());
	}

	template <class... Args>
	std::pair<iterator, bool> emplace(Args &&... args) {
		// Handle the annoying case where the type may not be moveable.
		value_type tmp(std::forward<Args>(args)...);
		auto I = lower_bound(tmp);
		if (I == end()) {
			storage.emplace(I, std::forward<Args>(args)...);
			return std::make_pair(I, true);
		} else if (equivalent(*I, tmp)) {
			return std::make_pair(I, false);
		} else {
			storage.emplace(I, std::forward<Args>(args)...);
			return std::make_pair(I, true);
		}
	}

	template <class... Args>
	iterator emplace_hint(const_iterator, Args &&... args) {
		// XXX: We can't use the hint efficiently because of the
		// contiguous nature of the storage. :)
		return emplace(std::forward<Args>(args)...).first;
	}

	// Search operations we need to implement
	size_type count(const key_type &key) const {
		auto result = equal_range(key);
		return std::distance(result.first, result.second);
	}

	iterator find(const Key &key) {
		auto I = lower_bound(key);
		if (I == end() || comp(key, *I))
			return I;
		return end();
	}
	const_iterator find(const Key &k) const {
		return const_cast<sorted_vector<Key, Compare> *>(this)->find(k);
	}

	iterator upper_bound(const Key &key) {
		return std::upper_bound(begin(), end(), key, comp_refwrap());
	}
	const_iterator upper_bound(const Key &key) const {
		return std::upper_bound(begin(), end(), key, comp_refwrap());
	}

	iterator lower_bound(const Key &key) {
		return std::lower_bound(begin(), end(), key, comp_refwrap());
	}
	const_iterator lower_bound(const Key &key) const {
		return std::lower_bound(begin(), end(), key, comp_refwrap());
	}

	std::pair<iterator, iterator> equal_range(const Key &key) {
		return std::equal_range(begin(), end(), key, comp_refwrap());
	}

	std::pair<const_iterator, const_iterator>
	equal_range(const Key &key) const {
		return std::equal_range(begin(), end(), key, comp_refwrap());
	}

	bool validate() {
		bool sorted = std::is_sorted(begin(), end(), comp_refwrap());
		auto f = [this](const value_type &a, const value_type &b)
			     ->bool { return this->equivalent(a, b); };
		bool unique = std::adjacent_find(begin(), end(), f) == end();
		return sorted && unique;
	}
};

// TODO: Implement the std::swap specialization
#endif
