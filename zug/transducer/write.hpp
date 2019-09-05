//
// zug: transducers for C++
// Copyright (C) 2019 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <zug/detail/copy_traits.hpp>
#include <zug/state_wrapper.hpp>
#include <zug/with_state.hpp>

#include <functional>
#include <ostream>

namespace zug {

namespace detail {

struct empty_output
{};

template <typename StreamT>
StreamT& operator<<(StreamT& stream, empty_output)
{
    return stream;
}

template <typename OutputStreamT, typename SepT>
void write_inputs(OutputStreamT&, SepT&& sep)
{}

template <typename OutputStreamT, typename SepT, typename InputT>
void write_inputs(OutputStreamT& stream, SepT&& sep, InputT&& in)
{
    stream << std::forward<InputT>(in);
}

template <typename OutputStreamT,
          typename SepT,
          typename InputT,
          typename... InputTs>
void write_inputs(OutputStreamT& stream,
                  SepT&& sep,
                  InputT&& in,
                  InputTs... ins)
{
    stream << std::forward<InputT>(in) << sep;
    write_inputs(stream, sep, std::forward<InputTs>(ins)...);
}

} // namespace detail

/*!
 * Transducer that writes the into a given @a `stream` using the
 * `operator <<`.  It also forwards the values for further processing.
 */
template <typename OutputStreamT, typename InSeparatorT, typename ArgSeparatorT>
auto write(OutputStreamT& stream, InSeparatorT in_sep, ArgSeparatorT arg_sep)
{
    return [=, stream_ref = std::ref(stream)](auto&& step) {
        return [=](auto&& s, auto&&... is) mutable {
            using std::begin;
            using std::end;

            using result_t = decltype(wrap_state(step(state_unwrap(s), is...)));
            using complete_t = decltype(state_complete(s));

            using state_t = decltype(s);
            using wrapped_t =
                detail::copy_decay_t<state_t, std::decay_t<result_t>>;
            using unwrapped_t =
                detail::copy_decay_t<state_t, std::decay_t<complete_t>>;

            auto& stream = stream_ref.get();
            return with_state(
                ZUG_FWD(s),
                [&](unwrapped_t&& st) {
                    detail::write_inputs(stream, arg_sep, is...);
                    return wrap_state(
                        step(std::forward<unwrapped_t>(st), ZUG_FWD(is)...));
                },
                [&](wrapped_t&& st) {
                    stream << in_sep;
                    detail::write_inputs(stream, arg_sep, is...);
                    return wrap_state(
                        step(state_unwrap(std::forward<wrapped_t>(st)),
                             ZUG_FWD(is)...));
                });
        };
    };
}

template <typename OutputStreamT, typename InSeparatorT = detail::empty_output>
auto write(OutputStreamT& stream, InSeparatorT in_sep = InSeparatorT{})
{
    return write(stream, in_sep, in_sep);
}

} // namespace zug