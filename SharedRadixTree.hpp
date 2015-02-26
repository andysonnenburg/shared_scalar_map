// Copyright 2015 The MathWorks, Inc.
#ifndef _eml_general_SharedRadixTree_hpp
#define _eml_general_SharedRadixTree_hpp

#include <tuple>
#include <type_traits>
#include <utility>

#include <cmath>
#include <cstdint>

#ifdef _WIN32
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#ifdef _WIN64
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

namespace EML
{
    namespace shared_radix_tree_detail
    {
#if defined(_WIN32)

        template <typename T>
        typename std::enable_if<
            sizeof(T) <= sizeof(unsigned long),
            unsigned long
            >::type log2(T arg)
        {
            unsigned long result;
            _BitScanReverse(&result, arg);
            return result;
        }

#if defined(_WIN64)

        template <typename T>
        typename std::enable_if<
            sizeof(unsigned long) < sizeof(T) && sizeof(T) <= sizeof(__int64),
            unsigned long
            >::type log2(T arg)
        {
            unsigned long result;
            _BitScanReverse64(&result, arg);
            return result;
        }

#endif

#elif defined(__GNUC__)

        template <typename T>
        typename std::enable_if<
            sizeof(T) <= sizeof(unsigned int),
            int
            >::type log2(T arg)
        {
          return sizeof(arg) * 8 - 1 - __builtin_clz(arg);
        }

        template <typename T>
        typename std::enable_if<
            sizeof(unsigned int) < sizeof(T) && sizeof(T) <= sizeof(unsigned long),
            int
            >::type log2(T arg)
        {
          return sizeof(arg) * 8 - 1 - __builtin_clzl(arg);
        }

        template <typename T>
        typename std::enable_if<
            sizeof(unsigned long) < sizeof(T) && sizeof(T) <= sizeof(unsigned long long),
            int
            >::type log2(T arg)
        {
          return sizeof(arg) * 8 - 1 - __builtin_clzll(arg);
        }

#endif

        template <typename>
        struct is_pointer_to_const;

        template <typename T>
        struct is_pointer_to_const<T const*>
            : std::true_type
        {};

        template <typename T>
        struct is_pointer_to_const<T*>
            : std::false_type
        {};

        template <typename>
        struct remove_pointer;

        template <typename T>
        struct remove_pointer<T*>
        {
            typedef T type;
        };

        /// A reference counting pointer.  The advantage of this over
        /// `std::shared_ptr` is both time and space.  Bumping a
        /// `std::shared_ptr` use count requires a `virtual` function call.
        /// Bumping an `intrusive_shared_ptr` use count is performed via a
        /// direct call.  A `std::shared_ptr` requires two words of space.
        /// `intrusive_shared_ptr` only requires one.  These advantages are at
        /// the cost of usability.  `intrusive_shared_ptr` requires any `T` to
        /// be a subclass of `control_block`.
        template <typename>
        struct intrusive_shared_ptr;

        /// Control block for `intrusive_shared_ptr` containing a use count.
        /// Only `intrusive_shared_ptr` may modify the use count.  To be able
        /// to be used with `intrusive_shared_ptr`, a type must subclass
        /// `control_block`.
        struct control_block
        {
            template <typename>
            friend struct intrusive_shared_ptr;

            control_block()
                : fUseCount(1)
            {}

            unsigned int use_count() const
            {
                return fUseCount;
            }

            bool unique() const
            {
                return fUseCount == 1;
            }

          private:
            unsigned int& use_count()
            {
                return fUseCount;
            }

            unsigned int fUseCount;
        };

        template <typename T>
        struct intrusive_shared_ptr
        {
            template <typename>
            friend struct intrusive_shared_ptr;

            intrusive_shared_ptr()
                : fPtr(nullptr)
            {}

            template <typename U>
            intrusive_shared_ptr(U* ptr)
                : fPtr(ptr)
            {}

            intrusive_shared_ptr(std::nullptr_t)
                : fPtr(nullptr)
            {}

            intrusive_shared_ptr(intrusive_shared_ptr const& rhs)
                : fPtr(rhs.fPtr)
            {
                if (auto block = get_control_block()) {
                    ++block->use_count();
                }
            }

            template <typename U>
            intrusive_shared_ptr(intrusive_shared_ptr<U> const& rhs)
                : fPtr(rhs.fPtr)
            {
                if (auto block = get_control_block()) {
                    ++block->use_count();
                }
            }

            intrusive_shared_ptr(intrusive_shared_ptr&& rhs)
                : fPtr(rhs.fPtr)
            {
                rhs.fPtr = nullptr;
            }

            template <typename U>
            intrusive_shared_ptr(intrusive_shared_ptr<U>&& rhs)
                : fPtr(rhs.fPtr)
            {
                rhs.fPtr = nullptr;
            }

            intrusive_shared_ptr& operator=(intrusive_shared_ptr rhs)
            {
                swap(rhs);
                return *this;
            }

            ~intrusive_shared_ptr()
            {
                if (auto block = get_control_block()) {
                    if (block->unique()) {
                        delete fPtr;
                    } else {
                        --block->use_count();
                    }
                }
            }

            T& operator*() const
            {
                return *fPtr;
            }

            T* operator->() const
            {
                return fPtr;
            }

            friend bool operator==(intrusive_shared_ptr const& lhs, intrusive_shared_ptr const& rhs)
            {
                return lhs.fPtr == rhs.fPtr;
            }

            friend bool operator!=(intrusive_shared_ptr const& lhs, intrusive_shared_ptr const& rhs)
            {
                return lhs.fPtr != rhs.fPtr;
            }

            // XXX
            // Replace with `explicit operator bool() const`.
            operator bool() const
            {
                return fPtr;
            }

            void swap(intrusive_shared_ptr& rhs)
            {
                using std::swap;
                swap(fPtr, rhs.fPtr);
            }

            bool unique() const
            {
                if (auto block = get_control_block()) {
                    return block->unique();
                }
                return true;
            }

          private:
            control_block* get_control_block() const
            {
                return static_cast<control_block*>(fPtr);
            }

            T* fPtr;
        };

        template <typename T>
        void swap(intrusive_shared_ptr<T>& lhs, intrusive_shared_ptr<T>& rhs)
        {
            lhs.swap(rhs);
        }

        template <
            typename T,
            typename Arg0,
            typename Arg1
            >
        intrusive_shared_ptr<T> make_shared(Arg0&& arg0, Arg1&& arg1)
        {
            return intrusive_shared_ptr<T>(new T(std::forward<Arg0>(arg0),
                                                 std::forward<Arg1>(arg1)));
        }

        template <
            typename T,
            typename Arg0,
            typename Arg1,
            typename Arg2,
            typename Arg3
            >
        intrusive_shared_ptr<T> make_shared(Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
        {
            return intrusive_shared_ptr<T>(new T(std::forward<Arg0>(arg0),
                                                 std::forward<Arg1>(arg1),
                                                 std::forward<Arg2>(arg2),
                                                 std::forward<Arg3>(arg3)));
        }

        template <typename ValueType>
        struct iterator
        {
            template <typename>
            friend struct iterator;

            iterator()
                : fValue(nullptr)
            {}

            template <typename OtherValueType>
            iterator(iterator<OtherValueType> const& i)
                : fValue(i.fValue)
            {}

            explicit iterator(ValueType* value)
                : fValue(value)
            {}

            ValueType& operator*() const
            {
                return *fValue;
            }

            ValueType* operator->() const
            {
                return fValue;
            }

            friend bool operator==(iterator const& lhs, iterator const& rhs)
            {
                return lhs.fValue == rhs.fValue;
            }

            friend bool operator!=(iterator const& lhs, iterator const& rhs)
            {
                return lhs.fValue != rhs.fValue;
            }

          private:
            ValueType* fValue;
        };

        /// If `key` is definitely not contained by a branch having prefix
        /// `prefix` and mask `mask`, return `true`.  This function may return
        /// false negatives.
        template <typename Key, typename Prefix, typename Mask>
        bool not_mem(Key const& key, Prefix const& prefix, Mask const& mask)
        {
            return (key & (~(mask - static_cast<Mask>(1)) ^ mask)) != prefix;
        }

        /// If `key` may be in the left node of a branch, return true.
        /// Otherwise, return false.
        template <typename Prefix, typename Mask>
        bool left(Prefix const& key, Mask const& mask)
        {
            return (key & mask) == static_cast<Prefix>(0);
        }

        /// The interface required by all nodes.
        template <typename, typename, typename, typename>
        struct node;

        /// A branch node containing a prefix, a mask, a left node, and a right
        /// node.
        template <typename, typename, typename, typename>
        struct branch;

        /// A leaf node containing a key-value pair.
        template <typename, typename, typename, typename>
        struct leaf;

        /// Construct a mask from two prefixes by finding the most significant
        /// bit the prefixes differ at.  Only this bit is set in the result
        /// mask.
        template <typename Mask, typename Prefix>
        Mask make_mask(Prefix prefix1, Prefix prefix2)
        {
            return static_cast<Mask>(1) << static_cast<Mask>(log2(prefix1 ^ prefix2));
        }

        /// Construct a prefix by zeroing out all the bits of the prefix at and
        /// below the single bit set on the mask.
        template <typename Prefix, typename Mask>
        Prefix make_prefix(Prefix prefix, Mask mask)
        {
            return prefix & (~(mask - static_cast<Mask>(1)) ^ mask);
        }

        /// Construct a `intrusive_shared_ptr` to a `branch` from two prefixes
        /// and two nodes.  This function determines which node should be the
        /// left node and which node should be the right node.
        template <typename Prefix, typename Mask, typename Key, typename T>
        intrusive_shared_ptr<branch<Key, T, Prefix, Mask>> make_branch(Prefix const& prefix1,
                                                                       intrusive_shared_ptr<node<Key, T, Prefix, Mask>> node1,
                                                                       Prefix const& prefix2,
                                                                       intrusive_shared_ptr<node<Key, T, Prefix, Mask>> node2)
        {
            auto mask = make_mask<Mask>(prefix1, prefix2);
            auto prefix = make_prefix(prefix1, mask);
            if (left(prefix1, mask)) {
                return make_shared<branch<Key, T, Prefix, Mask>>(prefix,
                                                                 mask,
                                                                 std::move(node1),
                                                                 std::move(node2));
            }
            return make_shared<branch<Key, T, Prefix, Mask>>(prefix,
                                                             mask,
                                                             std::move(node2),
                                                             std::move(node1));
        }

        template <typename This>
        struct find_result
            : std::conditional<is_pointer_to_const<This>::value,
                               typename remove_pointer<This>::type::const_iterator,
                               typename remove_pointer<This>::type::iterator>
        {};

        template <
            typename Key,
            typename T,
            typename Prefix,
            typename Mask
            >
        struct node : control_block
        {
            typedef node node_type;
            typedef branch<Key, T, Prefix, Mask> branch_type;
            typedef leaf<Key, T, Prefix, Mask> leaf_type;
            typedef Key key_type;
            typedef T mapped_type;
            typedef std::pair<key_type const, mapped_type> value_type;
            typedef shared_radix_tree_detail::iterator<value_type> iterator;
            typedef shared_radix_tree_detail::iterator<value_type const> const_iterator;
            typedef int size_type;

            virtual ~node() {}

            /// Insert a value under this node.  This node's
            /// `intrusive_shared_ptr` should be passed as an additional
            /// argument.  The result is a `std::tuple` containing a
            /// `intrusive_shared_ptr` that should replace this node, an
            /// `iterator` pointing to the found or inserted value, and a
            /// `bool` indicating if an insertion occurred.  The path up to but
            /// not necessarily including this node's `intrusive_shared_ptr` is
            /// required to be unique, possibly allowing for destructive
            /// update.
            /// @see insert_shared
            virtual std::tuple<intrusive_shared_ptr<node>, iterator, bool> insert_unique(intrusive_shared_ptr<node> const&, value_type const&) = 0;

            /// Similar to `insert_unique`, except the path up to this node is
            /// known to not be unique.  Therefore, destructive updates may not
            /// be used.
            /// @see insert_unique
            virtual std::tuple<intrusive_shared_ptr<node>, iterator, bool> insert_shared(intrusive_shared_ptr<node> const&, value_type const&) = 0;

            virtual iterator find(Key const&) = 0;

            virtual const_iterator find(Key const&) const = 0;

            virtual std::tuple<intrusive_shared_ptr<node>, size_type> erase_unique(intrusive_shared_ptr<node_type> const&, key_type const&) = 0;

            virtual std::tuple<intrusive_shared_ptr<node>, size_type> erase_shared(intrusive_shared_ptr<node_type> const&, key_type const&) = 0;
        };

        template <
            typename Key,
            typename T,
            typename Prefix,
            typename Mask
            >
        struct branch : node<Key, T, Prefix, Mask>
        {
            typedef node<Key, T, Prefix, Mask> node_type;
            using typename node_type::branch_type;
            using typename node_type::leaf_type;
            using typename node_type::key_type;
            using typename node_type::mapped_type;
            using typename node_type::value_type;
            using typename node_type::iterator;
            using typename node_type::const_iterator;
            using typename node_type::size_type;

            template <
                typename OtherPrefix,
                typename OtherMask,
                typename OtherLeft,
                typename OtherRight
                >
            branch(OtherPrefix&& prefix, OtherMask&& mask, OtherLeft&& left, OtherRight&& right)
                : fPrefix(std::forward<OtherPrefix>(prefix))
                , fMask(std::forward<OtherMask>(mask))
                , fLeft(std::forward<OtherLeft>(left))
                , fRight(std::forward<OtherRight>(right))
            {}

            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool> insert_unique(intrusive_shared_ptr<node_type> const& ptr, value_type const& value) override final
            {
                if (not_mem(value.first, fPrefix, fMask)) {
                    return insert_not_mem(ptr, value);
                }
                if (left(static_cast<Prefix>(value.first), fMask)) {
                    return insert_left(ptr, value);
                }
                return insert_right(ptr, value);
            }

            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool> insert_shared(intrusive_shared_ptr<node_type> const& ptr, value_type const& value) override final
            {
                if (not_mem(value.first, fPrefix, fMask)) {
                    return insert_not_mem(ptr, value);
                }
                if (left(static_cast<Prefix>(value.first), fMask)) {
                    return insert_left_shared(value);
                }
                return insert_right_shared(value);
            }

            iterator find(key_type const& key) override final
            {
                return find_impl(this, key);
            }

            const_iterator find(key_type const& key) const override final
            {
                return find_impl(this, key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type> erase_unique(intrusive_shared_ptr<node_type> const& ptr, key_type const& key) override final
            {
                if (not_mem(key, fPrefix, fMask)) {
                    return erase_not_mem(ptr);
                }
                if (left(static_cast<Prefix>(key), fMask)) {
                    return erase_left(ptr, key);
                }
                return erase_right(ptr, key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type> erase_shared(intrusive_shared_ptr<node_type> const& ptr, key_type const& key) override final
            {
                if (not_mem(key, fPrefix, fMask)) {
                    return erase_not_mem(ptr);
                }
                if (left(static_cast<Prefix>(key), fMask)) {
                    return erase_left_shared(key);
                }
                return erase_right_shared(key);
            }

          private:
            /// Insert `value` by constructing a new `branch` node and setting
            /// `this` node and a new `leaf` containing `value` under it.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_not_mem(intrusive_shared_ptr<node_type> ptr, value_type const& value) const
            {
                auto leaf = make_shared<leaf_type>(value.first, value.second);
                iterator i(&leaf->get());
                auto branch = make_branch(static_cast<Prefix>(value.first), intrusive_shared_ptr<node_type>(std::move(leaf)), fPrefix, std::move(ptr));
                return std::make_tuple(std::move(branch), i, true);
            }

            /// Insert `value` by inserting `value` in `fLeft`, destructively
            /// if `this` `ptr` is `unique`, non-destructively otherwise.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_left(intrusive_shared_ptr<node_type> const& ptr, value_type const& value)
            {
                if (ptr.unique()) {
                    return insert_left_unique(ptr, value);
                }
                return insert_left_shared(value);
            }

            /// Insert `value` by inserting `value` in `fLeft` destructively.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_left_unique(intrusive_shared_ptr<node_type> ptr, value_type const& value)
            {
                iterator i;
                bool inserted;
                std::tie(fLeft, i, inserted) = fLeft->insert_unique(fLeft, value);
                return std::make_tuple(std::move(ptr), i, inserted);
            }

            /// Insert `value` by inserting `value` in `fLeft` non-
            /// destructively.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_left_shared(value_type const& value) const
            {
                intrusive_shared_ptr<node_type> left;
                iterator i;
                bool inserted;
                std::tie(left, i, inserted) = fLeft->insert_shared(fLeft, value);
                auto branch = make_shared<branch_type>(fPrefix, fMask, std::move(left), fRight);
                return std::make_tuple(std::move(branch), i, inserted);
            }

            /// Insert `value` by inserting `value` in `fRight`, destructively
            /// if `this` `ptr` is `unique`, non-destructively otherwise.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_right(intrusive_shared_ptr<node_type> const& ptr, value_type const& value)
            {
                if (ptr.unique()) {
                    return insert_right_unique(ptr, value);
                }
                return insert_right_shared(value);
            }

            /// Insert `value` by inserting `value` in `fRight` destructively.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_right_unique(intrusive_shared_ptr<node_type> ptr, value_type const& value)
            {
                iterator i;
                bool inserted;
                std::tie(fRight, i, inserted) = fRight->insert_unique(fRight, value);
                return std::make_tuple(std::move(ptr), i, inserted);
            }

            /// Insert `value` by inserting `value` in `fRight` non-
            /// destructively.
            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool>
            insert_right_shared(value_type const& value) const
            {
                intrusive_shared_ptr<node_type> right;
                iterator i;
                bool inserted;
                std::tie(right, i, inserted) = fRight->insert_shared(fRight, value);
                auto branch = make_shared<branch_type>(fPrefix, fMask, fLeft, std::move(right));
                return std::make_tuple(std::move(branch), i, inserted);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_not_mem(intrusive_shared_ptr<node_type> ptr)
            {
                return std::make_tuple(std::move(ptr), 0);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_left(intrusive_shared_ptr<node_type> const& ptr, key_type const& key)
            {
                if (ptr.unique()) {
                    return erase_left_unique(ptr, key);
                }
                return erase_left_shared(key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_left_unique(intrusive_shared_ptr<node_type> const& ptr, key_type const& key)
            {
                intrusive_shared_ptr<node_type> left;
                size_type result;
                std::tie(left, result) = fLeft->erase_unique(fLeft, key);
                if (left) {
                    fLeft = std::move(left);
                    return std::make_tuple(ptr, result);
                }
                return std::make_tuple(fRight, result);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_left_shared(key_type const& key)
            {
                intrusive_shared_ptr<node_type> left;
                size_type result;
                std::tie(left, result) = fLeft->erase_shared(fLeft, key);
                if (left) {
                    auto branch = make_shared<branch_type>(fPrefix, fMask, std::move(left), fRight);
                    return std::make_tuple(std::move(branch), result);
                }
                return std::make_tuple(fRight, result);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_right(intrusive_shared_ptr<node_type> const& ptr, key_type const& key)
            {
                if (ptr.unique()) {
                    return erase_right_unique(ptr, key);
                }
                return erase_right_shared(key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_right_unique(intrusive_shared_ptr<node_type> const& ptr, key_type const& key)
            {
                intrusive_shared_ptr<node_type> right;
                size_type result;
                std::tie(right, result) = fRight->erase_unique(fRight, key);
                if (right) {
                    fRight = std::move(right);
                    return std::make_tuple(ptr, result);
                }
                return std::make_tuple(fLeft, result);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type>
            erase_right_shared(key_type const& key)
            {
                intrusive_shared_ptr<node_type> right;
                size_type result;
                std::tie(right, result) = fRight->erase_shared(fRight, key);
                if (right) {
                    auto branch = make_shared<branch_type>(fPrefix, fMask, fLeft, std::move(right));
                    return std::make_tuple(std::move(branch), result);
                }
                return std::make_tuple(fLeft, result);
            }

            /// Implementation of both `const` and non-`const` `find`.
            template <typename This>
            static typename find_result<This>::type find_impl(This aThis, key_type const& aKey)
            {
                if (not_mem(aKey, aThis->fPrefix, aThis->fMask)) {
                    return typename find_result<This>::type();
                }
                if (left(static_cast<Prefix>(aKey), aThis->fMask)) {
                    return aThis->fLeft->find(aKey);
                }
                return aThis->fRight->find(aKey);
            }

            /// If a key does not match `fPrefix` above the bit set by `fMask`,
            /// such a key cannot be contained under this node.
            Prefix fPrefix;
            /// If the bit of a key at `fMask`'s single set bit is `0`, the key
            /// may be contained in `fLeft`.  Otherwise, it may be contained
            /// in `fRight`.
            Mask fMask;
            intrusive_shared_ptr<node<key_type, mapped_type, Prefix, Mask>> fLeft;
            intrusive_shared_ptr<node<key_type, mapped_type, Prefix, Mask>> fRight;
        };

        template <
            typename Key,
            typename T,
            typename Prefix,
            typename Mask
            >
        struct leaf : node<Key, T, Prefix, Mask>
        {
            typedef node<Key, T, Prefix, Mask> node_type;
            using typename node_type::leaf_type;
            using typename node_type::key_type;
            using typename node_type::mapped_type;
            using typename node_type::value_type;
            using typename node_type::iterator;
            using typename node_type::const_iterator;
            using typename node_type::size_type;

            template <typename OtherKey, typename U>
            leaf(OtherKey&& key, U&& mapped)
                : fValue(std::forward<OtherKey>(key), std::forward<U>(mapped))
            {}

            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool> insert_unique(intrusive_shared_ptr<node_type> const& ptr, value_type const& value) override final
            {
                return insert_shared(ptr, value);
            }

            std::tuple<intrusive_shared_ptr<node_type>, iterator, bool> insert_shared(intrusive_shared_ptr<node_type> const& ptr, value_type const& value) override final
            {
                if (value.first == fValue.first) {
                    return std::make_tuple(ptr, iterator(&fValue), false);
                }
                auto leaf = make_shared<leaf_type>(value.first, value.second);
                iterator i(&leaf->get());
                auto branch = make_branch<Prefix>(value.first, intrusive_shared_ptr<node_type>(std::move(leaf)), fValue.first, ptr);
                return std::make_tuple(std::move(branch), i, true);
            }

            iterator find(key_type const& key) override final
            {
                return find_impl(this, key);
            }

            const_iterator find(key_type const& key) const override final
            {
                return find_impl(this, key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type> erase_unique(intrusive_shared_ptr<node_type> const& ptr, key_type const& key) override final
            {
                return erase_shared(ptr, key);
            }

            std::tuple<intrusive_shared_ptr<node_type>, size_type> erase_shared(intrusive_shared_ptr<node_type> const& ptr, key_type const& key) override final
            {
                if (key == fValue.first) {
                    return std::make_tuple(intrusive_shared_ptr<node_type>(), 1);
                }
                return std::make_tuple(ptr, 0);
            }

            value_type& get()
            {
                return fValue;
            }

            value_type const& get() const
            {
                return fValue;
            }

          private:
            /// Implementation of both `const` and non-`const` `find`.
            template <typename This>
            static typename find_result<This>::type find_impl(This aThis, key_type const& aKey)
            {
                if (aKey != aThis->fValue.first) {
                    return typename find_result<This>::type();
                }
                return typename find_result<This>::type(&aThis->fValue);
            }

            value_type fValue;
        };

        /// Prefix implementation for pointer types.
        struct ptr_prefix;

        /// Mask implementation for pointer types.
        struct ptr_mask;

        struct ptr_prefix
        {
            friend ptr_prefix operator&(ptr_prefix, ptr_mask);

            template <typename T>
            ptr_prefix(T* ptr)
                : fValue(reinterpret_cast<std::uintptr_t>(ptr))
            {}

            template <typename T>
            explicit ptr_prefix(T value)
                : fValue(static_cast<std::uintptr_t>(value))
            {}

            friend ptr_prefix operator^(ptr_prefix lhs, ptr_prefix rhs)
            {
                return ptr_prefix(lhs.fValue ^ rhs.fValue);
            }

            friend bool operator==(ptr_prefix lhs, ptr_prefix rhs)
            {
                return lhs.fValue == rhs.fValue;
            }

            friend bool operator!=(ptr_prefix lhs, ptr_prefix rhs)
            {
                return lhs.fValue != rhs.fValue;
            }

            std::uintptr_t get() const
            {
                return fValue;
            }

          private:
            std::uintptr_t fValue;
        };

        inline auto log2(ptr_prefix prefix) -> decltype(log2(prefix.get()))
        {
            return log2(prefix.get());
        }

        struct ptr_mask
        {
            friend ptr_prefix operator&(ptr_prefix, ptr_mask);

            template <typename T>
            explicit ptr_mask(T value)
                : fValue(static_cast<std::uintptr_t>(value))
            {}

            friend ptr_mask operator<<(ptr_mask lhs, ptr_mask rhs)
            {
                return ptr_mask(lhs.fValue << rhs.fValue);
            }

            friend ptr_mask operator-(ptr_mask lhs, ptr_mask rhs)
            {
                return ptr_mask(lhs.fValue - rhs.fValue);
            }

            ptr_mask operator~() const
            {
                return ptr_mask(~fValue);
            }

            friend ptr_mask operator^(ptr_mask lhs, ptr_mask rhs)
            {
                return ptr_mask(lhs.fValue ^ rhs.fValue);
            }

          private:
            std::uintptr_t fValue;
        };

        inline ptr_prefix operator&(ptr_prefix lhs, ptr_mask rhs)
        {
            return ptr_prefix(lhs.fValue & rhs.fValue);
        }

        template <
            typename Key,
            typename T,
            /// Prefix implementation.  This type must support common bit
            /// operations, implementation of `log2`, and explicit conversion
            /// from `Key`.
            /// @see ptr_prefix
            typename Prefix = typename std::conditional<
                std::is_pointer<Key>::value,
                ptr_prefix,
                Key>::type,
            /// Mask implementation.  This type must support common bit
            /// operations.
            /// @see ptr_mask
            typename Mask = typename std::conditional<
                std::is_pointer<Key>::value,
                ptr_mask,
                Key>::type
            >
        struct SharedRadixTree
        {
          private:
            typedef node<Key, T, Prefix, Mask> node_type;
            typedef typename node_type::branch_type branch_type;
            typedef typename node_type::leaf_type leaf_type;

          public:
            typedef typename node_type::key_type key_type;
            typedef typename node_type::mapped_type mapped_type;
            typedef typename node_type::value_type value_type;
            typedef typename node_type::iterator iterator;
            typedef typename node_type::const_iterator const_iterator;
            typedef typename node_type::size_type size_type;

            SharedRadixTree()
            {}

            /// `O(min(log(n), sizeof(Key)))`
            std::pair<iterator, bool> insert(value_type const& value)
            {
                if (fNode) {
                    std::pair<iterator, bool> result;
                    std::tie(fNode, result.first, result.second) = fNode->insert_unique(fNode, value);
                    return result;
                }
                auto leaf = make_shared<leaf_type>(value.first, value.second);
                iterator i(&leaf->get());
                fNode = std::move(leaf);
                return std::make_pair(i, true);
            }

            /// `O(min(log(n), sizeof(Key)))`
            iterator find(key_type const& key)
            {
                return find_impl(this, key);
            }

            /// `O(min(log(n), sizeof(Key)))`
            const_iterator find(key_type const& key) const
            {
                return find_impl(this, key);
            }

            /// `O(min(log(n), sizeof(Key)))`
            size_type erase(key_type const& key)
            {
                if (fNode) {
                    size_type result;
                    std::tie(fNode, result) = fNode->erase_unique(fNode, key);
                    return result;
                }
                return 0;
            }

            /// `O(1)`
            iterator end()
            {
                return iterator();
            }

            /// `O(1)`
            const_iterator end() const
            {
                return const_iterator();
            }

            /// `O(1)`
            const_iterator cend() const
            {
                return const_iterator();
            }

            /// `O(n)`
            void clear()
            {
                fNode = nullptr;
            }

            /// `O(1)`
            bool empty() const
            {
                return !fNode;
            }

          private:
            template <typename This>
            static typename find_result<This>::type find_impl(This aThis, key_type const& aKey)
            {
                if (aThis->fNode) {
                    return aThis->fNode->find(aKey);
                }
                return typename find_result<This>::type();
            }

            intrusive_shared_ptr<node_type> fNode;
        };
    }

    /// Map from `Key` to `T` implemented as a radix tree with path
    /// compression.  Insertion, lookup, and deletion are all nearly constant
    /// time, while copying is constant time.  The methods of this class follow
    /// the `AssociativeContainer` concept where possible.
    using shared_radix_tree_detail::SharedRadixTree;
}

#endif
