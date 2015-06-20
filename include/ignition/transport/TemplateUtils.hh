/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef __IGN_TRANSPORT_TEMPLATE_UTILS_HH_INCLUDED__
#define __IGN_TRANSPORT_TEMPLATE_UTILS_HH_INCLUDED__

namespace ignition
{
  namespace transport
  {
    /// \brief Template used to strip const.
    template <typename U> struct UnConst
    {
        typedef U Result;
    };

    /// \brief Template specialization to strip const.
    template <typename U> struct UnConst<const U&>
    {
        typedef U Result;
    };

    /// \brief Template specialization to strip const.
    template <typename U> struct UnConst<U&>
    {
       typedef U Result;
    };

    /// \brief Template for getting some function information:
    ///   * arity: Number of function parameters.
    ///   * argument_type<N>: Data type of argument N.
    template<typename FunctionT>
    struct function_traits
    {
      static constexpr std::size_t arity =
        function_traits<decltype(& FunctionT::operator())>::arity - 1;

      template<std::size_t N>
      using argument_type = typename function_traits<decltype(
        & FunctionT::operator())>::template argument_type<N + 1>;
    };

    /// \brief Template specialization.
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT(Args ...)>
    {
      static constexpr std::size_t arity = sizeof ... (Args);

      template<std::size_t N>
      using argument_type =
        typename std::tuple_element<N, std::tuple<Args ...>>::type;
    };

    /// \brief Template specialization.
    template<typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT (*)(Args ...)>
      : public function_traits<ReturnTypeT(Args ...)>
    {};

    /// \brief Template specialization.
    template<typename ClassT, typename ReturnTypeT, typename ... Args>
    struct function_traits<ReturnTypeT (ClassT::*)(Args ...) const>
      : public function_traits<ReturnTypeT(ClassT &, Args ...)>
    {};
  }
}
#endif
