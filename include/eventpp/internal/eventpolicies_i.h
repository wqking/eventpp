namespace internal_ {

template <typename T>
struct HasTypeArgumentPassingMode
{
	template <typename C> static std::true_type test(typename C::ArgumentPassingMode *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectArgumentPassingMode;
template <typename T> struct SelectArgumentPassingMode <T, true> { using Type = typename T::ArgumentPassingMode; };
template <typename T> struct SelectArgumentPassingMode <T, false> { using Type = ArgumentPassingAutoDetect; };

template <typename T>
struct HasTypeThreading
{
	template <typename C> static std::true_type test(typename C::Threading *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectThreading;
template <typename T> struct SelectThreading <T, true> { using Type = typename T::Threading; };
template <typename T> struct SelectThreading <T, false> { using Type = MultipleThreading; };

template <typename T>
struct HasTypeCallback
{
	template <typename C> static std::true_type test(typename C::Callback *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool, typename D> struct SelectCallback;
template <typename T, typename D> struct SelectCallback<T, true, D> { using Type = typename T::Callback; };
template <typename T, typename D> struct SelectCallback<T, false, D> { using Type = D; };

template <typename T, typename ...Args>
struct HasFunctionGetEvent
{
	template <typename C> static std::true_type test(decltype(C::getEvent(std::declval<Args>()...)) *);
	template <typename C> static std::false_type test(...);
	
	enum { value = !! decltype(test<T>(0))() };
};
template <typename E>
struct DefaultGetEvent
{
	template <typename U, typename ...Args>
	static E getEvent(U && e, Args && ...) {
		return e;
	}
};
template <typename T, typename Key, bool> struct SelectGetEvent;
template <typename T, typename Key> struct SelectGetEvent<T, Key, true> { using Type = T; };
template <typename T, typename Key> struct SelectGetEvent<T, Key, false> { using Type = DefaultGetEvent<Key>; };

template <typename T>
struct HasFunctionCanContinueInvoking
{
	template <typename C> static std::true_type test(decltype(&C::canContinueInvoking) *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
struct DefaultCanContinueInvoking
{
	template <typename ...Args>
	static bool canContinueInvoking(Args && ...) {
		return true;
	}
};
template <typename T, bool> struct SelectCanContinueInvoking;
template <typename T> struct SelectCanContinueInvoking<T, true> { using Type = T; };
template <typename T> struct SelectCanContinueInvoking<T, false> { using Type = DefaultCanContinueInvoking; };

template <typename T>
struct HasTemplateMap
{
	template <typename C> static std::true_type test(typename C::template Map<int, int> *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T>
class HasHash
{
	template <typename C> static std::true_type test(decltype(std::hash<C>()(std::declval<C>())) *) ;
	template <typename C> static std::false_type test(...);    

public:
	enum { value = !! decltype(test<T>(0))() };
};
template <typename Key, typename Value, typename T, bool>
struct SelectMap;
template <typename Key, typename Value, typename T>
struct SelectMap<Key, Value, T, true>
{
	using Type = typename T::template Map<Key, Value>;
};
template <typename Key, typename Value, typename T>
struct SelectMap<Key, Value, T, false> {
	using Type = typename std::conditional<
		HasHash<Key>::value,
		std::unordered_map<Key, Value>,
		std::map<Key, Value>
	>::type;
};


template <typename T>
struct HasTypeMixins
{
	template <typename C> static std::true_type test(typename C::Mixins *) ;
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};
template <typename T, bool> struct SelectMixins;
template <typename T> struct SelectMixins <T, true> { using Type = typename T::Mixins; };
template <typename T> struct SelectMixins <T, false> { using Type = MixinList<>; };


template <typename Root, typename TList>
struct InheritMixins;

template <typename Root, template <typename> class T, template <typename> class ...Args>
struct InheritMixins <Root, MixinList<T, Args...> >
{
	using Type = T <typename InheritMixins<Root, MixinList<Args...> >::Type>;
};

template <typename Root>
struct InheritMixins <Root, MixinList<> >
{
	using Type = Root;
};

template <typename Root, typename TList, typename Func>
struct ForEachMixins;

template <typename Func, typename Root, template <typename> class T, template <typename> class ...Args>
struct ForEachMixins <Root, MixinList<T, Args...>, Func>
{
	using Type = typename InheritMixins<Root, MixinList<T, Args...> >::Type;

	template <typename ...A>
	static bool forEach(A && ...args) {
		if(Func::template forEach<Type>(std::forward<A>(args)...)) {
			return ForEachMixins<Root, MixinList<Args...>, Func>::forEach(std::forward<A>(args)...);
		}
		return false;
	}
};

template <typename Root, typename Func>
struct ForEachMixins <Root, MixinList<>, Func>
{
	using Type = Root;

	template <typename ...A>
	static bool forEach(A && .../*args*/) {
		return true;
	}
};

template <typename T, typename ...Args>
struct HasFunctionMixinBeforeDispatch
{
	template <typename C> static std::true_type test(
		decltype(std::declval<C>().mixinBeforeDispatch(std::declval<Args>()...)) *
	);
	template <typename C> static std::false_type test(...);    

	enum { value = !! decltype(test<T>(0))() };
};

} //namespace internal_
