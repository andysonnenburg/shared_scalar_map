#ifndef _eml_general_shared_scalar_map_hpp
#define _eml_general_shared_scalar_map_hpp

#include <iostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <cmath>

namespace EML
{
namespace shared_scalar_map_detail
{
template <typename>
struct pointer_is_const;

template <typename T>
struct pointer_is_const<T const*>
  : std::true_type
{};

template <typename T>
struct pointer_is_const<T*>
  : std::false_type
{};

struct make_arg_t
{};

namespace
{
make_arg_t make_arg = {};
}

template <typename>
struct shared_ptr;

struct control_block
{
  template <typename>
  friend struct shared_ptr;

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
struct shared_ptr
{
  template <typename>
  friend struct shared_ptr;

  shared_ptr()
    : fPtr(nullptr)
  {}

  template <typename Arg0, typename Arg1>
  shared_ptr(make_arg_t, Arg0&& arg0, Arg1&& arg1)
    : fPtr(new T(std::forward<Arg0>(arg0), std::forward<Arg1>(arg1)))
  {}

  template <typename Arg0, typename Arg1, typename Arg2, typename Arg3>
  shared_ptr(make_arg_t, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
    : fPtr(new T(std::forward<Arg0>(arg0),
                 std::forward<Arg1>(arg1),
                 std::forward<Arg2>(arg2),
                 std::forward<Arg3>(arg3)))
  {}

  shared_ptr(std::nullptr_t)
    : fPtr(nullptr)
  {}

  shared_ptr(shared_ptr const& rhs)
    : fPtr(rhs.fPtr)
  {
    if (auto block = get_control_block()) {
      ++block->use_count();
    }
  }

  template <typename U>
  shared_ptr(shared_ptr<U> const& rhs)
    : fPtr(rhs.fPtr)
  {
    if (auto block = get_control_block()) {
      ++block->use_count();
    }
  }

  shared_ptr(shared_ptr&& rhs)
    : fPtr(rhs.fPtr)
  {
    rhs.fPtr = nullptr;
  }

  template <typename U>
  shared_ptr(shared_ptr<U>&& rhs)
    : fPtr(rhs.fPtr)
  {
    rhs.fPtr = nullptr;
  }

  shared_ptr& operator=(shared_ptr rhs)
  {
    swap(rhs);
    return *this;
  }

  ~shared_ptr()
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

  explicit operator bool() const
  {
    return fPtr;
  }

  void swap(shared_ptr& rhs)
  {
    using std::swap;
    swap(fPtr, rhs.fPtr);
  }

private:
  control_block* get_control_block() const
  {
    return static_cast<control_block*>(fPtr);
  }

  T* fPtr;
};

template <typename T>
void swap(shared_ptr<T>& lhs, shared_ptr<T>& rhs)
{
  lhs.swap(rhs);
}

template <
  typename T,
  typename Arg0,
  typename Arg1
  >
shared_ptr<T> make_shared(Arg0&& arg0, Arg1&& arg1)
{
  return shared_ptr<T>(make_arg,
                       std::forward<Arg0>(arg0),
                       std::forward<Arg1>(arg1));
}

template <
  typename T,
  typename Arg0,
  typename Arg1,
  typename Arg2,
  typename Arg3
  >
shared_ptr<T> make_shared(Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
{
  return shared_ptr<T>(make_arg,
                       std::forward<Arg0>(arg0),
                       std::forward<Arg1>(arg1),
                       std::forward<Arg2>(arg2),
                       std::forward<Arg3>(arg3));
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

template <typename Key, typename Prefix, typename Mask>
bool not_mem(Key const& key, Prefix const& prefix, Mask const& mask)
{
  return (key & (~(mask - 1) ^ mask)) != prefix;
}

template <typename Prefix, typename Mask>
bool left(Prefix const& key, Mask const& mask)
{
  return (key & mask) == 0;
}

template <typename, typename, typename, typename>
struct node;

template <typename, typename, typename, typename>
struct branch;

template <typename, typename, typename, typename>
struct leaf;

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
  typedef shared_scalar_map_detail::iterator<value_type> iterator;
  typedef shared_scalar_map_detail::iterator<value_type const> const_iterator;
  virtual ~node() {}
  virtual std::tuple<shared_ptr<node>, iterator, bool> insert(shared_ptr<node> const&, value_type const&) = 0;
  virtual iterator find(Key const&) = 0;
  virtual const_iterator find(Key const&) const = 0;
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

  std::tuple<shared_ptr<node_type>, iterator, bool> insert(shared_ptr<node_type> const& ptr, value_type const& value)
  {
    if (not_mem(value.first, fPrefix, fMask)) {
      return insert_not_mem(ptr, value);
    }
    if (left(value.first, fMask)) {
      return insert_left(value);
    }
    return insert_right(value);
  }

  iterator find(key_type const& key) override final
  {
    return find_impl(this, key);
  }

  const_iterator find(key_type const& key) const override final
  {
    return find_impl(this, key);
  }

private:
  std::tuple<shared_ptr<node_type>, iterator, bool>
  insert_not_mem(shared_ptr<node_type> ptr, value_type const& value)
  {
    auto leaf = make_shared<leaf_type>(value.first, value.second);
    iterator i(&leaf->get());
    auto branch = make_branch(value.first, shared_ptr<node_type>(std::move(leaf)), fPrefix, std::move(ptr));
    return std::make_tuple(std::move(branch), i, true);
  }

  std::tuple<shared_ptr<node_type>, iterator, bool>
  insert_left(value_type const& value)
  {
    shared_ptr<node_type> left;
    iterator i;
    bool inserted;
    std::tie(left, i, inserted) = fLeft->insert(fLeft, value);
    auto branch = make_shared<branch_type>(fPrefix, fMask, std::move(left), fRight);
    return std::make_tuple(std::move(branch), i, inserted);
  }

  std::tuple<shared_ptr<node_type>, iterator, bool>
  insert_right(value_type const& value)
  {
    shared_ptr<node_type> right;
    iterator i;
    bool inserted;
    std::tie(right, i, inserted) = fRight->insert(fRight, value);
    auto branch = make_shared<branch_type>(fPrefix, fMask, fLeft, std::move(right));
    return std::make_tuple(std::move(branch), i, inserted);
  }

  template <typename This>
  struct find_result
    : std::conditional<pointer_is_const<This>::value,
                       const_iterator,
                       iterator>
  {};

  template <typename This>
  static typename find_result<This>::type find_impl(This aThis, key_type const& aKey)
  {
    if (not_mem(aKey, aThis->fPrefix, aThis->fMask)) {
      return typename find_result<This>::type();
    }
    if (left(aKey, aThis->fMask)) {
      return aThis->fLeft->find(aKey);
    }
    return aThis->fRight->find(aKey);
  }

  Prefix fPrefix;
  Mask fMask;
  shared_ptr<node<key_type, mapped_type, Prefix, Mask>> fLeft;
  shared_ptr<node<key_type, mapped_type, Prefix, Mask>> fRight;
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

  template <typename OtherKey, typename U>
  leaf(OtherKey&& key, U&& mapped)
    : fValue(std::forward<OtherKey>(key), std::forward<U>(mapped))
  {}

  std::tuple<shared_ptr<node_type>, iterator, bool> insert(shared_ptr<node_type> const& ptr, value_type const& value)
  {
    if (value.first == fValue.first) {
      return std::make_tuple(ptr, iterator(&fValue), false);
    }
    auto leaf = make_shared<leaf_type>(value.first, value.second);
    iterator i(&leaf->get());
    auto branch = make_branch(value.first, shared_ptr<node_type>(std::move(leaf)), fValue.first, ptr);
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

  value_type& get()
  {
    return fValue;
  }

  value_type const& get() const
  {
    return fValue;
  }

private:
  template <typename This>
  struct find_result
    : std::conditional<pointer_is_const<This>::value,
                       const_iterator,
                       iterator>
  {};

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

template <typename Mask, typename Prefix>
Mask make_mask(Prefix prefix1, Prefix prefix2)
{
  using std::log2;
  return 1 << static_cast<Mask>(log2(prefix1 ^ prefix2));
}

template <typename Prefix, typename Mask>
Prefix make_prefix(Prefix prefix, Mask mask)
{
  return prefix & (~(mask - 1) ^ mask);
}

template <typename Key, typename T, typename Prefix, typename Mask>
shared_ptr<branch<Key, T, Prefix, Mask>>
make_branch(Prefix const& prefix1,
            shared_ptr<node<Key, T, Prefix, Mask>> node1,
            Prefix const& prefix2,
            shared_ptr<node<Key, T, Prefix, Mask>> node2)
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

template <
  typename Key,
  typename T,
  typename Prefix = T,
  typename Mask = T
  >
struct shared_scalar_map
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

public:
  shared_scalar_map()
  {}

  std::pair<iterator, bool> insert(value_type const& value)
  {
    if (fNode) {
      std::pair<iterator, bool> result;
      std::tie(fNode, result.first, result.second) = fNode->insert(fNode, value);
      return result;
    }
    auto leaf = make_shared<leaf_type>(value.first, value.second);
    iterator i(&leaf->get());
    fNode = std::move(leaf);
    return std::make_pair(i, true);
  }

  iterator find(key_type const& key)
  {
    return find_impl(this, key);
  }

  const_iterator find(key_type const& key) const
  {
    return find_impl(this, key);
  }

  iterator end()
  {
    return iterator();
  }

  const_iterator end() const
  {
    return const_iterator();
  }

  const_iterator cend() const
  {
    return const_iterator();
  }

  void clear()
  {
    fNode = nullptr;
  }

  bool empty() const
  {
    return !fNode;
  }

private:
  template <typename This>
  struct find_result
    : std::conditional<pointer_is_const<This>::value,
                       const_iterator,
                       iterator>
  {};

  template <typename This>
  static typename find_result<This>::type find_impl(This aThis, key_type const& aKey)
  {
    if (aThis->fNode) {
      return aThis->fNode->find(aKey);
    }
    return aThis->end();
  }

  shared_ptr<node_type> fNode;
};
}

using shared_scalar_map_detail::shared_scalar_map;
}

#endif
