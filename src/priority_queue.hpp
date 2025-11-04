#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T val;
		Node *left;
		Node *right;
		int dist; // null-path length for leftist heap
		Node(const T &v) : val(v), left(nullptr), right(nullptr), dist(1) {}
	};

	Node *root = nullptr;
	std::size_t cnt = 0;
	Compare comp{};

	static int npl(Node *x) { return x ? x->dist : 0; }

	// Merge (meld) two leftist heaps. Strong exception safety for comparator throws.
	Node *meld(Node *a, Node *b) {
		if (!a) return b;
		if (!b) return a;
		// Comparator may throw; we must not modify any node state before it completes.
		if (comp(a->val, b->val)) { Node *t = a; a = b; b = t; }
		// Recurse to attach b into a->right; no modifications to 'a' happen until recursion returns successfully.
		Node *nr = meld(a->right, b);
		a->right = nr;
		// Maintain leftist property
		if (npl(a->left) < npl(a->right)) { Node *t = a->left; a->left = a->right; a->right = t; }
		a->dist = npl(a->right) + 1;
		return a;
	}

	static void destroy(Node *x) {
		if (!x) return;
		destroy(x->left);
		destroy(x->right);
		delete x;
	}

	static Node *clone(Node *x) {
		if (!x) return nullptr;
		Node *n = new Node(x->val);
		n->dist = x->dist;
		n->left = clone(x->left);
		n->right = clone(x->right);
		return n;
	}

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() = default;

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(nullptr), cnt(other.cnt), comp(other.comp) {
		root = clone(other.root);
	}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() { destroy(root); }

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		// Copy-construct a temporary for strong exception safety
		priority_queue tmp(other);
		// Swap internals (manual swaps to avoid extra headers)
		Node *tr = root; root = tmp.root; tmp.root = tr;
		std::size_t tc = cnt; cnt = tmp.cnt; tmp.cnt = tc;
		// Comparator state should match 'other'
		comp = other.comp;
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (empty()) throw container_is_empty();
		return root->val;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		Node *n = nullptr;
		try {
			// Allocate first; if allocation throws, nothing changes.
			n = new Node(e);
			Node *new_root = meld(root, n);
			root = new_root;
			++cnt;
		} catch (...) {
			// If meld threw, 'n' has not been linked yet; delete to avoid leak, then rethrow.
			delete n;
			throw;
		}
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (empty()) throw container_is_empty();
		Node *old = root;
		Node *L = root->left;
		Node *R = root->right;
		try {
			Node *new_root = meld(L, R);
			root = new_root;
			--cnt;
			delete old;
		} catch (...) {
			// On comparator throw, no structural modifications occurred; keep queue intact.
			throw;
		}
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const { return cnt; }

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const { return cnt == 0; }

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other) return; // merging with self: no-op
		Node *r1 = root;
		Node *r2 = other.root;
		try {
			Node *new_root = meld(r1, r2);
			root = new_root;
			cnt += other.cnt;
			other.root = nullptr;
			other.cnt = 0;
		} catch (...) {
			// On comparator throw, both heaps must remain unchanged.
			throw;
		}
	}
};

}

#endif