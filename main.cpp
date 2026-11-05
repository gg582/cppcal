import std;

constexpr auto nth_bit_is_high = [](unsigned int x, unsigned int n) constexpr noexcept -> bool {
    return (x & (1u << n)) != 0;
};

const std::string EMPTY_STRING = "";
const auto DELIMETER = ';';

using NumType = std::variant<long long, double>;

std::unordered_map<std::string, NumType> vars;

std::vector<char> alreadyUsed{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '=', '+', '-', '*', '/'};

std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return EMPTY_STRING;
    auto end = s.find_last_not_of(" \t\r\n");
    auto substr_endpos = end - start + 1;
    return s.substr(start, substr_endpos);
}

template <typename T1, typename T2, typename Op>
concept CalculableSame = std::same_as<T1, T2> && requires(T1 a, T2 b, Op op) {
    // Requires that T1 and T2 are identical types,
    // and invoking op(a, b) yields the same type T1 without implicit conversion.
    { op(a, b) } -> std::same_as<T1>;
};

template <typename Op>
NumType applyBinaryOp(const NumType &l, const NumType &r, Op op) {
    return std::visit([op](auto &&a, auto &&b) -> NumType {
        using T1 = std::decay_t<decltype(a)>;
        using T2 = std::decay_t<decltype(b)>;
        using CleanOp = std::decay_t<Op>;

        if constexpr (CalculableSame<T1, T2, CleanOp>) {
            return op(a, b);
        } else {
            std::cerr << "err: type mismatch" << std::endl;
            std::exit(1);
        }
    }, l, r);
}

NumType evalExp(const std::string &rawExpr);

NumType evalOp(std::string tok) {
    tok = trim(tok);

    // float(...) expression
    if (tok.size() >= 7 && tok.rfind("float(", 0) == 0 && tok.back() == ')') {
        std::string inner = tok.substr(6, tok.size() - 7);
        // NOTE: recursive call
        NumType v = evalExp(inner);
        return std::visit([](auto &&val) -> NumType {
            return static_cast<double>(val);
        }, v);
    }

    if (vars.contains(tok)) {
        return vars[tok];
    }

    // if an expression contains ".",
    // we assume it's double.
    // For example, 10.0 is regarded as double in here.
    if (tok.find('.') != std::string::npos)
        return std::stod(tok);

    // if we found no "." from an expression, we assume it's long long.
    return std::stoll(tok);
}

NumType evalExp(const std::string &rawExpr) {
    std::string expr = trim(rawExpr);

    // Scan for top-level binary operators outside parentheses
    int parenDepth = 0;
    size_t plusPos = std::string::npos;
    size_t minusPos = std::string::npos;
    size_t mulPos = std::string::npos;
    size_t divPos = std::string::npos;

    for (size_t i = 0; i < expr.size(); ++i) {
        char ch = expr[i];
        if (ch == '(') {
            parenDepth++;
        } else if (ch == ')') {
            parenDepth--;
        } else if (parenDepth == 0) {
            if (ch == '+') plusPos = i;
            else if (ch == '-') minusPos = i;
            else if (ch == '*') mulPos = i;
            else if (ch == '/') divPos = i;
        }
    }

    unsigned int opMask = 0;

    /** NOTE: This should handle only one expression per execution
     * =======OPERATION TABLE===========
     *
     * | Operation | Bit |
     * | :-------- | :-- |
     * |     +     | 0th |
     * |     -     | 1th |
     * |     *     | 2nd |
     * |     /     | 3rd |
     *
     * ================================= */
    opMask = plusPos  != std::string::npos ? opMask | (1u << 0) : opMask;
    opMask = minusPos != std::string::npos ? opMask | (1u << 1) : opMask;
    opMask = mulPos   != std::string::npos ? opMask | (1u << 2) : opMask;
    opMask = divPos   != std::string::npos ? opMask | (1u << 3) : opMask;

    if (!opMask) return evalOp(expr);

    if ((opMask & (opMask - 1)) != 0) {
        std::cerr << "err: multiple operations not supported" << std::endl;
        std::exit(1);
    }

    size_t splitPos = std::string::npos;
    if (nth_bit_is_high(opMask, 0)) splitPos = plusPos;
    else if (nth_bit_is_high(opMask, 1)) splitPos = minusPos;
    else if (nth_bit_is_high(opMask, 2)) splitPos = mulPos;
    else if (nth_bit_is_high(opMask, 3)) splitPos = divPos;

    std::string lStr = expr.substr(0, splitPos);
    std::string rStr = expr.substr(splitPos + 1);

    NumType lhs = evalExp(lStr);
    NumType rhs = evalExp(rStr);

    if (nth_bit_is_high(opMask, 0)) return applyBinaryOp(lhs, rhs, std::plus<>{});
    if (nth_bit_is_high(opMask, 1)) return applyBinaryOp(lhs, rhs, std::minus<>{});
    if (nth_bit_is_high(opMask, 2)) return applyBinaryOp(lhs, rhs, std::multiplies<>{});
    if (nth_bit_is_high(opMask, 3)) {
        bool isZero = std::visit([](auto &&v) { return v == 0; }, rhs);
        if (isZero) {
            std::cerr << "err: division by zero" << std::endl;
            std::exit(1);
        }
        return applyBinaryOp(lhs, rhs, std::divides<>{});
    }

    return evalOp(expr);
}

void execStatement(const std::string &raw) {
    std::string stmt = trim(raw);

    if (stmt.empty()) return;

    // NOTE: print statement is not guarded.
    // for example, print x is fine.
    // but print(x) is not okay.
    if (stmt.rfind("print", 0) == 0) {
        std::string target = trim(stmt.substr(5));

        if (!vars.contains(target)) {
            std::cerr << "err: undefined variable" << std::endl;
            std::exit(1);
        }

        std::visit([](auto &&val) {
            std::cout << val << std::endl;
        }, vars[target]);
        return;
    }

    auto eqPos = stmt.find('=');

    // all statements EXCEPT print should follow:
    // name = value
    if (eqPos == std::string::npos) {
        std::cerr << "err: wrong statement" << std::endl;
        std::exit(1);
    }

    std::string var_name = trim(stmt.substr(0, eqPos));
    std::string expr = trim(stmt.substr(eqPos + 1));

    vars[var_name] = evalExp(expr);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: ./cppcal \"syntax1;syntax2;\"" << std::endl;
        std::cerr << "example: ./cppcal \"a=float(10);b=10.2;c=a+b;print c\"" << std::endl;
        return 1;
    }

    std::stringstream ss(argv[1]);
    std::string stmt;

    // currently delimeter is ';'
    // you can change it above.
    // however, when you use '.' or some characters those are already used for other syntaxes,
    // it will return an error.
    auto it = std::find(alreadyUsed.begin(), alreadyUsed.end(), DELIMETER);

    if (it != alreadyUsed.end()) return 1;
    while (std::getline(ss, stmt, DELIMETER)) {
        execStatement(stmt);
    }

    return 0;
}
