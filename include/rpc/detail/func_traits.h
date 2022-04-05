#pragma once

#ifndef FUNC_TRAITS_H_HWIWA6G0
#define FUNC_TRAITS_H_HWIWA6G0

#include "rpc/detail/bool.h"

namespace rpc {
namespace detail {

template <int N>
using is_zero = invoke<std::conditional<(N == 0), true_, false_>>;

template <int N, typename... Ts>
using nth_type = invoke<std::tuple_element<N, std::tuple<Ts...>>>;

namespace tags {

// tags for the function traits, used for tag dispatching
struct zero_arg {};
struct nonzero_arg {};
struct void_result {};
struct nonvoid_result {};
struct no_refs_args {};
struct refs_args {};

template <int N> struct arg_count_trait { typedef nonzero_arg type; };

template <> struct arg_count_trait<0> { typedef zero_arg type; };

template <typename T> struct result_trait { typedef nonvoid_result type; };

template <> struct result_trait<void> { typedef void_result type; };


}

//! \brief Provides a small function traits implementation that
//! works with a reasonably large set of functors.
template <typename T>
struct func_traits : func_traits<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...)> : func_traits<R (*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...) const> : func_traits<R (*)(Args...)> {};
template<size_t Index,typename T> struct element_holder;

template<size_t Index,typename T> struct element_holder<Index,T&>:std::integral_constant<size_t,Index>
{
    using type = T;
};



template<typename...T> struct RefArgsProducer;

template<typename...Touter> struct RefArgsProducer<std::tuple<Touter...>>
{
    template<typename...T> struct ReprojectTuple;

    template<typename...TInner,size_t...idxs> struct ReprojectTuple<std::tuple<TInner...>,std::index_sequence<idxs...> >
    {
        static std::tuple<TInner...> Reproject(std::tuple<Touter...>&& outer)
        {
            return ReprojectImpl(std::move(std::get<idxs>(outer))...);
        }

        static std::tuple<TInner...> ReprojectImpl(Touter&&... outer)
        {
            return std::make_tuple(outer...);
        }
    };

    


    template<typename...T> struct RefArgsPointer;
    template<typename...TInner> struct RefArgsPointer<std::tuple<TInner...>>
    {
        using pointer_idxs= std::index_sequence_for<TInner...>;
        using input_type = std::tuple<TInner...>;
        using tuple_type = RefArgsPointer<std::tuple<TInner...>,pointer_idxs>::tuple_type ;
        using sequence_type = RefArgsPointer<std::tuple<TInner...>,pointer_idxs>::sequence_type ;
        static tuple_type GetReffedValuesAsTuple(std::tuple<Touter&&...> input)
        {
            return ReprojectTuple<tuple_type,sequence_type>::Reproject(input);            
        }
    };

    template<typename...TInner,size_t...idxs> struct RefArgsPointer<std::tuple<TInner...>,std::index_sequence<idxs...> >
    {
        using input_type = std::tuple<TInner...>;
        using tuple_type = std::tuple< 
                        typename std::tuple_element_t<idxs,
                                            input_type
                                            >:: type...
                                    > ;
        using tuple_refs_type = std::tuple< 
                        typename std::tuple_element_t<idxs,
                                            input_type
                                            >:: type&...
                                    >;
        
        using sequence_type = std::index_sequence<std::tuple_element_t<idxs,
                                            input_type
                                            >:: value...>;

        
    };

    

};


template<typename Head,typename... Rest> struct ReferenceTupleElementHandler;


template<   typename T,
            typename T1,
            typename... T2
            > struct ReferenceTupleElementHandlerImplRoot;

template<typename T1,typename...T2> struct TupleVoidRemover;

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t<   (std::tuple_size_v< std::tuple<T1,T2...> > == 1) 
                                                    && (!std::is_void_v<T1>)
                                                > 
                            >
{
    using type=std::tuple<T1>;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t<   (std::tuple_size_v< std::tuple<T1,T2...> > == 1) 
                                                    && (std::is_void_v<T1>)
                                                > 
                            >
{
    using type=void;
};

template<typename T1,typename...T2,size_t...idxs> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::index_sequence<idxs...> 
                            >
{
    using type = std::tuple< std::tuple_element_t<idxs,std::tuple<T1,T2...> >... >;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t< (std::tuple_size_v< std::tuple<T1,T2...> > > 1) 
                                                && std::is_void_v<
                                                                std::tuple_element_t< 
                                                                                    std::tuple_size_v< 
                                                                                                    std::tuple<T1,T2...> 
                                                                                                    > - 1, 
                                                                                    std::tuple<T1,T2...> 
                                                                                    > 
                                                                > 
                                                > 
                            >
{
    using tuple_type = std::tuple<T1,T2...>;
    using type= TupleVoidRemover<tuple_type,std::make_index_sequence< std::tuple_size_v< tuple_type > -1 > >::type;
};

template<typename T1,typename...T2> 
    struct TupleVoidRemover<    std::tuple<T1,T2...>,
                                std::enable_if_t< 
                                                (std::tuple_size_v< std::tuple<T1,T2...> > > 1) 
                                                &&  !std::is_void_v<
                                                                    std::tuple_element_t< 
                                                                                    std::tuple_size_v< std::tuple<T1,T2...> > - 1, 
                                                                                    std::tuple<T1,T2...> 
                                                                                        > 
                                                                    > 
                                                    
                                                > 
                            >
{
    using type=std::tuple<T1,T2...>;
};

template<typename Head,typename... Rest> struct ReferenceTupleElementHandler<std::tuple<Head,Rest...> >
{
    using preliminary_type = ReferenceTupleElementHandlerImplRoot<
                                                std::integral_constant<size_t ,1+sizeof...(Rest)>,
                                                std::tuple<Head,Rest...>,
                                                void
                                            >::type;
    using type = TupleVoidRemover<
                                preliminary_type,
                                void
                                >::type;
};

template<> struct ReferenceTupleElementHandler<void >
{
    
    using type = void;
};

template<typename T> struct is_tuple : std::false_type{};
template<typename... Ts> struct is_tuple< std::tuple<Ts...> > : std::true_type{};

template<   size_t TLen,
            typename T1
            > struct ReferenceTupleElementHandlerImplRoot<std::integral_constant<size_t ,TLen>,T1,std::enable_if_t< !is_tuple<T1>::value && std::is_reference_v<T1> > >
{
    using type = std::tuple<element_holder<TLen-1,T1>>;
};

template<   size_t TLen,
            typename T1
            > struct ReferenceTupleElementHandlerImplRoot<std::integral_constant<size_t ,TLen>,T1,std::enable_if_t< !is_tuple<T1>::value && !std::is_reference_v<T1> > >
{
    using type = std::tuple<void>;
};


template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< sizeof...(Rest)==0 > >
{
    using type = ReferenceTupleElementHandlerImplRoot< std::integral_constant<size_t ,TLen>,T1,void >::type;
};

template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< std::is_reference_v<T1> && (sizeof...(Rest) > 0) > >
{
    using additional_type=ReferenceTupleElementHandlerImplRoot<   std::integral_constant<size_t , TLen>,
                                                                    std::tuple<Rest...>,
                                                                    void >::type;
    using this_element_type = element_holder<TLen - 1 - sizeof...(Rest),T1>;                                                                    
    using type = decltype(std::tuple_cat(std::declval<std::tuple<this_element_type>>(),std::declval<additional_type>()));// std::tuple<this_element_type,additional_type>  ;
};

template<   size_t TLen,
            typename T1,
            typename... Rest
            > struct ReferenceTupleElementHandlerImplRoot<  std::integral_constant<size_t ,TLen>,
                                                            std::tuple<T1,Rest...>,
                                                            std::enable_if_t< !std::is_reference_v<T1>  && (sizeof...(Rest) > 0) > >
{
//    using type = element_holder<TLen - 1 - sizeof...(Rest),T1>;
    using type = ReferenceTupleElementHandlerImplRoot<   std::integral_constant<size_t , TLen>,
                                                                    std::tuple<Rest...>,
                                                                    void >::type  ;
};




template<typename T1,typename...T > struct ResultTraitsImpl;

template<typename Res> struct ResultTraitsImpl<Res ,std::enable_if_t< !std::is_reference_v<Res> > >
{
    typedef Res type;
};

template<typename... Args> struct pack_has_references
{
    static constexpr bool value {(std::is_lvalue_reference_v<Args> || ...)};
};


template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t< std::is_void_v<Res> && pack_has_references<Args...>::value >,Args...  >
{
    typedef std::tuple<Args...>  type;
};

template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t<  !pack_has_references<Args...>::value >,Args...  >
{
    typedef Res  type;
};

template<typename Res,typename... Args  > struct ResultTraitsImpl< Res ,std::enable_if_t< !std::is_void_v<Res> && pack_has_references<Args...>::value >,Args...  >
{
    typedef std::tuple<Res,Args...>  type;
};



template<typename T,typename...T1> struct ResultTraits : ResultTraitsImpl<T,void,T1...>{};
template <typename R, typename... Args> struct func_traits<R (*)(Args...)> {
    using merged_args_type = ResultTraits<R,Args...>::type;
    using result_type = R;
    using refs_args_type = ReferenceTupleElementHandler< merged_args_type > ::type;
    using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
    using args_type = std::tuple<typename std::decay<Args>::type...>;
};

template <typename T>
struct func_kind_info : func_kind_info<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct func_kind_info<R (C::*)(Args...)> : func_kind_info<R (*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct func_kind_info<R (C::*)(Args...) const>
    : func_kind_info<R (*)(Args...)> {};

template <typename R, typename... Args> struct func_kind_info<R (*)(Args...)> {
    typedef typename tags::arg_count_trait<sizeof...(Args)>::type args_kind;
    typedef typename tags::result_trait<R>::type result_kind;
};

template <typename F> using is_zero_arg = is_zero<func_traits<F>::arg_count>;

template <typename F>
using is_single_arg =
    invoke<std::conditional<func_traits<F>::arg_count == 1, true_, false_>>;

template <typename F>
using is_void_result = std::is_void<typename func_traits<F>::result_type>;

template <typename F>
using has_ref_args = invoke<std::conditional< !std::is_void_v<typename func_traits<F>::refs_args_type> , true_, false_>>;
}
}

#endif /* end of include guard: FUNC_TRAITS_H_HWIWA6G0 */
