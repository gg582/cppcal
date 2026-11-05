import std;

#define NTH_BIT_IS_HIGH(x, n) ((x) & (1<<n))

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

template <typename T1, typename T2>
concept CalculableSame = std::same_as<T1, T2> && requires(T1 a, T2 b) {
    // NOTE: In my opinion, checking + and / will be enough/
    // - can be interpreted as (a) + (-b).
    // * can be interpreted as (a) * (b) == (a) + (a) + (a) + ... b times.
    // and if a + b has a same type, b definitely has a same type.
    // so (a) * (b) won't be in a wrong result.
    // But a / b is kinda confusing.
    // if we think about a = 1;b = 3;
    //
    // we guess that c = a / b; should be 0.333333333333333314829616256247390992939472198486328125,
    // But C++ doesn't care if its mathematical value is an integer or not,
    // so it's interpreted as ZERO by its language spec.
    // it's exactly same in C.
    // As a result, we don't need to calculate +, -, *, / in this scope.
    // just checking a, b, and a+b are in a same type will be perfectly okay.
    {a + b} -> std::same_as<T1>;
};

NumType addNumType(const NumType &l, const NumType &r) {
    return std::visit([](auto &&a, auto &&b) -> NumType {
        using T1 = std::decay_t<decltype(a)>;
        using T2 = std::decay_t<decltype(b)>;

        if constexpr (CalculableSame<T1, T2>) {
            return a + b;
        } else {
            std::cerr << "err: type mismatch" << std::endl;
            std::exit(1);
        }
    }, l, r);
}

NumType minusNumType(const NumType &l, const NumType &r) {
    return std::visit([](auto &&a, auto &&b) -> NumType {
            using T1 = std::decay_t<decltype(a)>;
            using T2 = std::decay_t<decltype(b)>;

            if constexpr (CalculableSame<T1, T2>) {
                return a - b;
            } else {
                std::cerr << "err: type mismatch" << std::endl;
                std::exit(1);
            }
    }, l, r);
}

NumType mulNumType(const NumType &l, const NumType &r) {
    return std::visit([](auto &&a, auto &&b) -> NumType {
            using T1 = std::decay_t<decltype(a)>;
            using T2 = std::decay_t<decltype(b)>;

            if constexpr (CalculableSame<T1, T2>) {
                return a * b;
            } else {
                std::cerr << "err: type mismatch" << std::endl;
                std::exit(1);
            }
    }, l, r);
}

NumType divNumType(const NumType &l, const NumType &r) {
    return std::visit([](auto &&a, auto &&b) -> NumType {
            using T1 = std::decay_t<decltype(a)>;
            using T2 = std::decay_t<decltype(b)>;

            if constexpr (CalculableSame<T1, T2>) {
                return a / b;
            } else {
                std::cerr << "err: type mismatch" << std::endl;
                std::exit(1);
            }
    }, l, r);
}

NumType evalOp(std::string tok) {
    tok = trim(tok);

    // float(...) expression
    if (tok.rfind("float(", 0) == 0
     && tok.back() == ')') {
        std::string inner = tok.substr(6, tok.size() - 7);
        // NOTE: recursive call
        NumType v = evalOp(inner);
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

NumType evalExp(const std::string &expr) {
    auto plusPos = expr.find('+');
    auto minusPos = expr.find('-');
    auto mulPos = expr.find('*');
    auto divPos = expr.find('/');

    int opMask = 0;

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
    opMask = plusPos  != std::string::npos ? opMask | (1 << 0) : opMask;
    opMask = minusPos != std::string::npos ? opMask | (1 << 1) : opMask;
    opMask = mulPos   != std::string::npos ? opMask | (1 << 2) : opMask;
    opMask = divPos   != std::string::npos ? opMask | (1 << 3) : opMask;

    if (!opMask) return evalOp(expr);

    if ((opMask & (opMask - 1)) != 0) {
        std::cerr << "err: multiple operations not supported" << std::endl;
        std::exit(1);
    }

    // exp should be 1 here

    // lStr '+' rStr
    if (NTH_BIT_IS_HIGH(opMask, 0)) {
        std::string lStr = expr.substr(0, plusPos);
        std::string rStr = expr.substr(plusPos + 1);
        // interpret them a number, evalOp will determine if it's double or long long
        // so it's basically x+y;
        return addNumType(evalOp(lStr),
                          evalOp(rStr));
    }

    if (NTH_BIT_IS_HIGH(opMask, 1)) {
        std::string lStr = expr.substr(0, minusPos);
        std::string rStr = expr.substr(minusPos + 1);
        return minusNumType(evalOp(lStr),
                            evalOp(rStr));
    }

    if (NTH_BIT_IS_HIGH(opMask, 2)) {
        std::string lStr = expr.substr(0, mulPos);
        std::string rStr = expr.substr(mulPos + 1);
        return mulNumType(evalOp(lStr),
                          evalOp(rStr));
    }

    if (NTH_BIT_IS_HIGH(opMask, 3)) {
        std::string lStr = expr.substr(0, divPos);
        std::string rStr = expr.substr(divPos + 1);
        return divNumType(evalOp(lStr),
                          evalOp(rStr));
    }

    return evalOp(expr);
}

void execStatement(const std::string &raw) {
    std::string stmt = trim(raw);

    if(stmt.empty()) return;

    // NOTE: print statement is not guarded.
    // for example, print x is fine.
    // but print(x) is not okay.
    if (stmt.rfind("print", 0) == 0) {
        std::string target = trim(stmt.substr(5));

        if(!vars.contains(target)) {
            std::cerr << "err: undefined variable" << std::endl;
            std::exit(1);
        }

        std::visit([](auto &&val) {
            std::cout << val << std::endl; }, vars[target]);
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

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::cerr << "usage: ./cppcal \"syntax1;syntax2;\"" << std::endl;
        std::cerr << "example: ./cppcal a=float(10);b=10.2;c=a+b;print c";
        return 1;
    }

    std::stringstream ss(argv[1]);
    std::string stmt;

    // currently delimeter is ';'
    // you can change it above.
    // however, when you use '.' or some characters those are already used for other syntaxes,
    // it will return an error.
    auto it = std::find(alreadyUsed.begin(), alreadyUsed.end(), DELIMETER);

    if(it != alreadyUsed.end()) return 1;
    while (std::getline(ss, stmt, DELIMETER)) {
        execStatement(stmt);
    }

    return 0;
}
