#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>

namespace math{
	class term{
	public:
		enum type{
			val, var,                           //a single value, x
			add, sub, mul, div, mod, pow,       //operators
			sqrt,
			sin, cos, tan,                      //trigonometric functions
			ln, log,                            //logarithmic functions
			abs
		};
		term();
		term(float v);
		term(term *t1, int type, term *t2);
		term(const term &t1, int type, const term &t2);
		term(term *other);
		term(const term &other);
		term(std::string source);
		~term();
		int findMatch(int pos, std::string source);

		void clear();
		static term fromValue(float v);
		static std::string typeStr(int type);
		void dump(std::string start = "");
		std::string toString(float v);
		std::string print();

		term& operator=(const term& other);
		float operator()(float x = 0.0f);
		float calc(float x = 0.0f);

		term derivative();
		void deflate();
		term deflated();
	private:
		term *f1 = nullptr, *f2 = nullptr;
		int type = 0;
		float value = 0.0f;
		friend bool operator< (const term& lhs, const term& rhs);
		friend bool operator==(const term& lhs, const term& rhs);
	};
	inline bool operator< (const term& lhs, const term& rhs){ return lhs.type < rhs.type; }
	inline bool operator> (const term& lhs, const term& rhs){ return rhs < lhs; }
	inline bool operator<=(const term& lhs, const term& rhs){ return !(lhs > rhs); }
	inline bool operator>=(const term& lhs, const term& rhs){ return !(lhs < rhs); }
	inline bool operator==(const term& lhs, const term& rhs){
		return (lhs.type == rhs.type) &&
			   (lhs.type == term::val ? lhs.value == rhs.value : true) &&
			   ((lhs.f1 && rhs.f1) ? *lhs.f1 == *rhs.f1 : true) &&
			   ((lhs.f2 && rhs.f2) ? *lhs.f2 == *rhs.f2 : true);
	}
	inline bool operator!=(const term& lhs, const term& rhs){ return !(lhs==rhs); }

	inline term var(){return term(0, term::var, 0);}
	inline term add(term t1, term t2){ return term(t1, term::add, t2);}
	inline term sub(term t1, term t2){ return term(t1, term::sub, t2);}
	inline term mul(term t1, term t2){ return term(t1, term::mul, t2);}
	inline term div(term t1, term t2){ return term(t1, term::div, t2);}

	inline std::ostream& operator<<(std::ostream &os, term t){
		os<<t.print();
		return os;
	}
}
