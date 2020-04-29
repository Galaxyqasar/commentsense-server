#pragma once

template <std::size_t index, typename type>
class tupleval {
public:
	tupleval(type const &v) {
		val = v;
	}
	tupleval(type &&v) {
		val = std::move(v);
	}
	tupleval() : val() {}
	type &get() {
		return val;
	}
private:
	type val;
};

template <std::size_t index, typename... elements>
class tuplebase {
};

template <std::size_t index, typename type, typename... elements>
class tuplebase<index, type, elements...> : public tupleval<index, type>, public tuplebase<index + 1, elements...> {
public:
	tuplebase() : tupleval<index, type>(), tuplebase<index + 1, elements...>() {}
	template <typename CL, typename ... CArgs>
	tuplebase(CL &&arg, CArgs &&... args) : tupleval<index, CL>(std::forward<CL>(arg)), 
											tuplebase<index + 1, elements...>(std::forward<CArgs>(args)...) {}
};

template <std::size_t index, typename L, typename... Args>
struct extract_type_at {
	using type = typename extract_type_at<index - 1, Args...>::type;
};

template <typename L, typename... Args>
struct extract_type_at<0, L, Args...> {
	using type = L;
};

template <typename... elements>
class tuple : public tuplebase<0, elements...> {
public:
	template <typename ...CArgs>
	tuple(CArgs &&... args) : tuplebase<0, elements...>(std::forward<CArgs>(args)...) {}
	
	template<std::size_t index>
	auto& get() {
		return (static_cast<tupleval<index, typename extract_type_at<index, elements...>::type> &>(*this)).get();
	}
};