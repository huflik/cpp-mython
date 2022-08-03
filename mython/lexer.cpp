#include "lexer.h"

using namespace std;

namespace parse {

	bool operator==(const Token& lhs, const Token& rhs) {
		using namespace token_type;

		if (lhs.index() != rhs.index()) {
			return false;
		}
		if (lhs.Is<Char>()) {
			return lhs.As<Char>().value == rhs.As<Char>().value;
		}
		if (lhs.Is<Number>()) {
			return lhs.As<Number>().value == rhs.As<Number>().value;
		}
		if (lhs.Is<String>()) {
			return lhs.As<String>().value == rhs.As<String>().value;
		}
		if (lhs.Is<Id>()) {
			return lhs.As<Id>().value == rhs.As<Id>().value;
		}
		return true;
	}

	bool operator!=(const Token& lhs, const Token& rhs) {
		return !(lhs == rhs);
	}

	std::ostream& operator<<(std::ostream& os, const Token& rhs) {
		using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

		VALUED_OUTPUT(Number);
		VALUED_OUTPUT(Id);
		VALUED_OUTPUT(String);
		VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

		UNVALUED_OUTPUT(Class);
		UNVALUED_OUTPUT(Return);
		UNVALUED_OUTPUT(If);
		UNVALUED_OUTPUT(Else);
		UNVALUED_OUTPUT(Def);
		UNVALUED_OUTPUT(Newline);
		UNVALUED_OUTPUT(Print);
		UNVALUED_OUTPUT(Indent);
		UNVALUED_OUTPUT(Dedent);
		UNVALUED_OUTPUT(And);
		UNVALUED_OUTPUT(Or);
		UNVALUED_OUTPUT(Not);
		UNVALUED_OUTPUT(Eq);
		UNVALUED_OUTPUT(NotEq);
		UNVALUED_OUTPUT(LessOrEq);
		UNVALUED_OUTPUT(GreaterOrEq);
		UNVALUED_OUTPUT(None);
		UNVALUED_OUTPUT(True);
		UNVALUED_OUTPUT(False);
		UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

		return os << "Unknown token :("sv;
	}

	Lexer::Lexer(std::istream& input) : input_(input) {
		NextToken();
	}

	const Token& Lexer::CurrentToken() const {
		return current_token_;
	}

	Token Lexer::NextToken() {
		DiscardUseless();
		const char c = input_.peek();
		Token token;
		if (line_indent_ != indent_) {
			token = ReadIndent();
		}
		else if (c == EOF) {
			if (!(no_tokens_ || CurrentToken().Is<token_type::Dedent>() ||
				CurrentToken().Is<token_type::Newline>() || CurrentToken().Is<token_type::Eof>())) {
				token = token_type::Newline{};
			}
			else {
				token = token_type::Eof{};
			}
		}
		else if (c == '\n') {
			token = token_type::Newline{};
			input_.ignore();
		}
		else if (std::isdigit(c)) {
			token = ReadNumber();
		}
		else if (std::isalpha(c) || c == '_') {
			token = ReadId();
		}
		else if (c == '\'' || c == '\"') {
			token = ReadString();
		}
		else if (c == '!' || c == '<' || c == '>' || c == '=') {
			input_.ignore();
			const char next_c = input_.peek();
			if (next_c == '=') {
				std::stringstream ss;
				ss << c << next_c;
				token = map_lexems.at(ss.str());
				input_.ignore();
			}
			else {
				token = token_type::Char{ c };
			}
		}
		else {
			token = token_type::Char{ c };
			input_.ignore();
		}
		if (no_tokens_) {
			no_tokens_ = false;
		}
		current_token_ = token;
		return token;
	}

	token_type::Number Lexer::ReadNumber() {
		std::stringstream ss;
		while (std::isdigit(input_.peek())) {
			ss << static_cast<char>(input_.get());
		}
		return token_type::Number{ std::stoi(ss.str()) };
	}

	token_type::String Lexer::ReadString() {
		std::stringstream ss;
		const char first = input_.get();
		char c;
		while ((c = input_.get()) != first) {
			if (c == '\\') {
				const char next_c = input_.get();
				switch (next_c)
				{
				case '\'':
				case '\"':
					ss << next_c;
					break;
				case 'n':
					ss << '\n';
					break;
				case 't':
					ss << '\t';
					break;
				default:
					ss << c << next_c;
					break;
				}
			}
			else {
				ss << c;
			}
		}
		return token_type::String{ ss.str() };
	}

	Token Lexer::ReadId() {
		std::stringstream ss;
		for (char c = input_.peek(); std::isalpha(c) || std::isdigit(c) || c == '_'; c = input_.peek()) {
			ss << static_cast<char>(input_.get());
		}
		std::string id{ ss.str() };
		if (map_lexems.count(id)) {
			return map_lexems.at(id);
		}
		else {
			return token_type::Id{ id };
		}
	}

	Token Lexer::ReadIndent() {
		Token token;
		if (line_indent_ > indent_) {
			indent_ = line_indent_;
			token = token_type::Indent{};
		}
		else {
			indent_ -= 2;
			token = token_type::Dedent{};
		}
		return token;
	}

	void Lexer::DiscardUseless() {
		int space_count = 0;

		while (true) {
			while (input_.peek() == ' ') {
				input_.ignore();
				++space_count;
			}
			if (input_.peek() == '#') {
				while (input_.peek() != '\n' && input_.peek() != EOF) {
					input_.ignore();
				}
			}
			if (input_.peek() != '\n' || !(no_tokens_ || CurrentToken().Is<token_type::Newline>())) {
				if (!no_tokens_ && CurrentToken().Is<token_type::Newline>() && space_count != indent_) {
					line_indent_ = space_count;
				}
				break;
			}			
			input_.ignore();
			space_count = 0;
		}
	}

}  // namespace parse