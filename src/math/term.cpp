#include "term.hpp"

namespace math{
	const char *operators = "+-*/%^";   // mathematical operators
	std::vector<std::pair<std::string, float>> constants = {{"e",2.71828f},{"pi",3.14159f}};

	term::term(){
		;
	}

	term::term(float v){
		type = val;
		value = v;
	}
	term::term(term *t1, int type, term *t2){
		f1 = new term(t1);
		this->type = type;
		f2 = new term(t2);
	}

	term::term(const term &t1, int type, const term &t2){
		f1 = new term(t1);
		this->type = type;
		f2 = new term(t2);
	}

	term::term(term *other){
		if(other){
			type = other->type;
			value = other->value;
			if(other->f1)
				f1 = new term(other->f1);
			if(other->f2)
				f2 = new term(other->f2);
		}
	}

	term::term(const term &other){
		type = other.type;
		value = other.value;
		if(other.f1)
			f1 = new term(other.f1);
		if(other.f2)
			f2 = new term(other.f2);
	}

	term::term(std::string source){   //parse source
		size_t s = 0;
		float x = 0.0f;
		try{
			x = std::stof(source, &s);
		}
		catch(const std::invalid_argument& ia){}

		if(source == "x" || source == "t"){
			type = term::var;
			return;
		}
		else if(s == source.length()){
			type = val;
			value = x;
			return;
		}
		else if(source[0] == '|' && source[source.length()-1] == '|'){
			type = abs;
			std::string section = std::string(source.begin() + 1, source.end() - 1);
			if(section[0] == '(' && findMatch(0, section) == int(section.length()) - 1)
				section = std::string(section.begin() + 1, section.end() - 1);
			f1 = new term(section);
			return;
		}
		else{
			// t1 [+-*/%^] t2
			for(unsigned k = 0; k < 6; k++){
				for(unsigned i = 0; i < source.length(); i++){
					if(source[i] == operators[k]){
						type = add + k;

						std::string first(source.begin(), source.begin() + i);
						if(first[0] == '(' && findMatch(0, first) == int(first.length() - 1))
							first = std::string(first.begin() + 1, first.end() - 1);

						std::string second(source.begin() + i + 1, source.end());
						if(second[0] == '(' && findMatch(0, second) == int(second.length() - 1))
							second = std::string(second.begin() + 1, second.end() - 1);

						f1 = new term(first);
						f2 = new term(second);
						return;
					}
					else if(source[i] == '('){
						i = findMatch(i, source);
					}
				}
			}
		}
		// f(x)
		if(source.find("sqrt(") == 0 && source.rfind(")") == source.length()-1){
			type = sqrt;
			f1 = new term(std::string(source.begin() + 4, source.end() - 1));
		}
		else if(source.find("sin(") == 0 && source.rfind(")") == source.length()-1){
			type = sin;
			f1 = new term(std::string(source.begin() + 4, source.end() - 1));
		}
		else if(source.find("cos(") == 0 && source.rfind(")") == source.length()-1){
			type = cos;
			f1 = new term(std::string(source.begin() + 4, source.end() - 1));
		}
		else if(source.find("tan(") == 0 && source.rfind(")") == source.length()-1){
			type = tan;
			f1 = new term(std::string(source.begin() + 4, source.end() - 1));
		}
		else{// a constant
			type = val;
			for(unsigned i = 0; i < constants.size(); i++){
				if(constants[i].first == source){
					value = constants[i].second;
				}
			}
			std::cout<<"constant found: "<<source<<" "<<value<<std::endl;
		}
	}

	int term::findMatch(int pos, std::string source){
		int depth = 1;
		while(depth > 0){
			char c = source[++pos];
			if(c == '(')
				depth++;
			else if(c == ')')
				depth--;
			else if(c == '\0')
				break;
		}
		return pos;
	}

	term::~term(){
		clear();
	}

	void term::clear(){
		if(f1)
			delete f1;
		if(f2)
			delete f2;
		f1 = f2 = nullptr;
	}

	term term::fromValue(float v){
		term t;
		t.value = v;
		t.type = val;
		return t;
	}

	term term::derivative(){
		term df1 = f1 ? f1->derivative() : nullptr;
		term df2 = f2 ? f2->derivative() : nullptr;

		term df;

		switch(type){
			case val: df = term(term::fromValue(0.0f)); break;
			case var: df = term(term::fromValue(1.0f)); break;
			case add: df = term(df1, add, df2); break;
			case mul: df = term(term(df1, mul, f2), add, term(f1, mul, df2)); break;
			case pow: {
				if(f1->type == val && f2->type == var)
					df = term(term(f1, ln, nullptr), mul, term(f1, pow, f2));
				else if(f1->type == var && f2->type == val)
					df = term(f2, mul, term(f1, pow, term::fromValue(f2->value - 1.0f)));
			} break;
		}
		return df.deflated();
	}

	std::string term::typeStr(int type){
		switch(type){
			case val: return "val";
			case var: return "var";
			case add: return "+";
			case sub: return "-";
			case mul: return "*";
			case div: return "/";
			case mod: return "%";
			case pow: return "^";

			case sqrt: return "sqrt";
			case sin: return "sin";
			case cos: return "cos";
			case tan: return "tan";
			case ln : return "ln";
			case log: return "log";
			case abs: return "abs";
			default: return "";
		}
	}

	void term::dump(std::string start){
		std::string s = (type == var) ? std::string("x") : (type == val) ? toString(value) : std::string();
		std::cout<<start<<"-term("<<typeStr(type)<<") "<<s<<std::endl;
		if(f1)
			f1->dump(start + " |");
		if(f2)
			f2->dump(start + " |");
	}

	std::string term::toString(float v){
		std::stringstream ss;
		ss<<v;
		return ss.str();
	}

	std::string term::print(){
		std::string str;
		if(type >= sqrt)
			str += "(";
		if(f1 && type > var){
			if(f1->type < type && f1->type >= add && type < sqrt){
				str+= std::string("(") + f1->print() + std::string(")");
			}
			else{
				str+= f1->print();
			}
		}
		if(type == val)
			str += toString(value);
		else if(type == var)
			str += "x";
		else if(type < sqrt)
			str += typeStr(type);
		else if(type >= sqrt){
			str = typeStr(type) + str;
			str += ")";
		}
		if(f2 && type > var){
			if(f2->type < type && f2->type >= add && f2->type < sqrt){
				str += std::string("(") + f2->print() + std::string(")");
			}
			else{
				str += f2->print();
			}
		}
		return str;
	}

	term& term::operator=(const term& other){
		clear();
		type = other.type;
		value = other.value;
		if(other.f1)
			f1 = new term(other.f1);
		if(other.f2)
			f2 = new term(other.f2);
		return *this;
	}

	float term::operator()(float x){
		return calc(x);
	}

	float term::calc(float x){
		switch(type){
			case val: return value;
			case var: return x;

			case add: return f1->calc(x) + f2->calc(x);
			case sub: return f1->calc(x) - f2->calc(x);
			case mul: return f1->calc(x) * f2->calc(x);
			case div: return f1->calc(x) / f2->calc(x);
			case pow: return ::pow(f1->calc(x), f2->calc(x));
			case mod: return int(f1->calc(x)) % int(f2->calc(x));

			case sqrt: return ::sqrt(f1->calc(x));
			case sin: return ::sin(f1->calc(x));
			case cos: return ::cos(f1->calc(x));
			case tan: return ::tan(f1->calc(x));
			case ln : return ::log(f1->calc(x));
			//case log: return ::logab(f1->calc(x), f2->calc(x));
			case abs: return ::abs(f1->calc(x));
			default: return 0;
		}
	}

	void term::deflate(){
		if(f1)
			f1->deflate();
		if(f2)
			f2->deflate();

		if(type == add){
			// 0+x -> x
			if(f1->type == val && f1->value == 0.0f){
				this->operator=(term(f2));
			}
			// x+0 -> x
			else if(f2->type == val && f2->value == 0.0f){
				this->operator=(term(f1));
			}
		}
		if(type == mul){
			//x*0 -> 0
			if((f1->type == val && f1->value == 0.0f) || (f2->type == val && f2->value == 0.0f)){
				type = val;
				value = 0.0f;
				clear();
			}
			//2*3*x -> 6*x
			else if(f1->type == val && f2->type == mul && f2->f1->type == val){
				float v = f1->value;
				this->operator=(term(f2));
				f1->value *= v;
			}
		}		
		//x^r
		if(type == pow && f2->type == val){
			//x^1 -> x
			if(f2->type == val && f2->value == 1.0f){
				this->operator=(term(f1));
			}
			//x^0 -> 1
			else if(f2->value == 0.0f){
				type = val;
				value = 1.0f;
				clear();
			}
		}
		//sqrt(4) -> 2 ,...
		if((f1 && f1->type == val) && type >= sqrt){
			value = calc();
			type = val;
			clear();
		}
		//1*2 -> 2 ,...
		if((f1 && f1->type == val) && (f2 && f2->type == val) && type < sqrt){
			value = calc();
			type = val;
			clear();
		}
		//(a/c)+(b/c) -> (a+b)/c
		if((f1 && f1->type == div) && (f2 && f2->type == div) && (*f1->f2 == *f2->f2) && type < sqrt){
			this->operator=(term(term(term(f1->f1), type, term(f2->f1)), div, term(f1->f2)));
		}
		//(x+x) -> 2*x
		//(n*x)/n -> x
		//TODO: order from special -> simple
	}

	term term::deflated(){
		term t(this);
		t.deflate();
		return t;
	}
}