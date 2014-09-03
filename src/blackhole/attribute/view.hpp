#pragma once

#include <array>
#include <iostream>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/mpl/set.hpp>
#include <boost/optional.hpp>

#include "blackhole/attribute/set.hpp"
#include "blackhole/config.hpp"
#include "blackhole/detail/iterator/join.hpp"
#include "blackhole/detail/traits/tuple.hpp"

namespace blackhole {

namespace attribute {

namespace set_view {

template<class... T>
struct tuple_extractor_t;

} // namespace set_view

class set_view_t {
    template<class... T> friend struct set_view::tuple_extractor_t;

public:
    typedef aux::iterator::join_t<set_t, true> const_iterator;

    struct attached_set_t { set_t v; };
    struct internal_set_t { set_t v; };
    struct external_set_t { set_t v; };

private:
    attached_set_t attached;  // Likely empty.
    internal_set_t internal;  // About level + message + pid + tid + timestamp (4-5).
    external_set_t external;  // The most filled (scoped + user attributes)

public:
    set_view_t() = default;

    set_view_t(set_t attached, set_t external, set_t&& internal) :
        attached({ std::move(attached) }),
        internal({ std::move(internal) }),
        external({ std::move(external) })
    {}

    bool
    empty() const BLACKHOLE_NOEXCEPT {
        return internal.v.empty() && external.v.empty() && attached.v.empty();
    }

    //! Intentionally allow to insert only into external attribute set.
    void insert(pair_t pair) {
        external.v.insert(std::move(pair));
    }

    //! Intentionally allow to insert only into external attribute set.
    template<typename InputIterator>
    void insert(InputIterator first, InputIterator last) {
        external.v.insert(first, last);
    }

    const_iterator begin() const BLACKHOLE_NOEXCEPT {
        std::array<set_t const*, 3> array {{ &internal.v, &external.v, &attached.v }};
        return const_iterator(array);
    }

    const_iterator end() const BLACKHOLE_NOEXCEPT {
        std::array<set_t const*, 3> array {{ &internal.v, &external.v, &attached.v }};
        return const_iterator(array, aux::iterator::invalidate_tag);
    }

    boost::optional<const attribute_t&>
    find(const std::string& name) const BLACKHOLE_NOEXCEPT {
        auto it = internal.v.find(name);
        if (it != internal.v.end()) {
            return it->second;
        }

        it = external.v.find(name);
        if (it != external.v.end()) {
            return it->second;
        }

        it = attached.v.find(name);
        if (it != attached.v.end()) {
            return it->second;
        }

        return boost::optional<const attribute_t&>();
    }

    const attribute_t& at(const std::string& name) const {
        auto value = find(name);
        if (!value) {
            throw std::out_of_range(name);
        }

        return *value;
    }
};

namespace set_view {

template<>
struct tuple_extractor_t<set_view_t::attached_set_t> {
    static
    std::tuple<const set_view_t::attached_set_t*>
    extract(const set_view_t& view) {
        return std::make_tuple(&view.attached);
    }
};

template<>
struct tuple_extractor_t<set_view_t::internal_set_t> {
    static
    std::tuple<const set_view_t::internal_set_t*>
    extract(const set_view_t& view) {
        return std::make_tuple(&view.internal);
    }
};

template<>
struct tuple_extractor_t<set_view_t::external_set_t> {
    static
    std::tuple<const set_view_t::external_set_t*>
    extract(const set_view_t& view) {
        return std::make_tuple(&view.external);
    }
};

template<class T, class Arg, class... Args>
struct tuple_extractor_t<T, Arg, Args...> {
    static
    std::tuple<const T*, const Arg*, const Args*...>
    extract(const set_view_t& view) {
        return std::tuple_cat(
            tuple_extractor_t<T>::extract(view),
            tuple_extractor_t<Arg, Args...>::extract(view)
        );
    }
};

} // namespace set_view

template<class T, unsigned size>
struct all_empty {
    static bool empty(const T& tuple) {
        return std::get<size>(tuple)->v.empty() && all_empty<T, size - 1>::empty(tuple);
    }
};

template<class T>
struct all_empty<T, 0u> {
    static bool empty(const T& tuple) {
        return std::get<0>(tuple)->v.empty();
    }
};

template<class T>
struct tuple_empty {
    static bool empty(const T& tuple) {
        return all_empty<T, std::tuple_size<T>::value - 1>::empty(tuple);
    }
};

template<class... T>
class partial_view_t {
public:
    typedef std::tuple<const T*...> tuple_type;
    typedef std::integral_constant<
        std::size_t,
        std::tuple_size<tuple_type>::value
    > tuple_size;

    typedef set_view_t::const_iterator const_iterator;

private:
    typedef std::array<const set_t*, tuple_size::value> array_type;
    typedef typename make_index_tuple<tuple_size::value>::type index_tuple_type;

    const tuple_type tuple;

public:
    partial_view_t(const set_view_t& view) :
        tuple(set_view::tuple_extractor_t<T...>::extract(view))
    {}

    bool
    empty() const BLACKHOLE_NOEXCEPT {
        return tuple_empty<tuple_type>::empty(tuple);
    }

    const_iterator
    begin() const BLACKHOLE_NOEXCEPT {
        return const_iterator(to_array(tuple, index_tuple_type()));
    }

    const_iterator
    end() const BLACKHOLE_NOEXCEPT {
        return const_iterator(to_array(tuple, index_tuple_type()), aux::iterator::invalidate_tag);
    }

private:
    template<unsigned... I>
    static
    inline
    array_type
    to_array(const tuple_type& tuple, index_tuple<I...>) {
        return array_type {{ &std::get<I>(tuple)->v... }};
    }
};

} // namespace attribute

} // namespace blackhole
