#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>


#define MAX_WORDS    8
#define MAX_WORD_LEN 32
#define MAX_LETTERS  26
#define MAX_LINE     256

typedef struct {
    char op;
    char lhs[MAX_WORDS][MAX_WORD_LEN];
    int  lhs_n;
    char rhs[MAX_WORD_LEN];
} Puzzle;

typedef struct {
    char letters[MAX_LETTERS];
    int  n;
    int  leading[MAX_LETTERS];
} LetterSet;

static void strip(char *s) {
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static int parse_puzzle(const char *input, Puzzle *p) {
    char buf[MAX_LINE];
    strncpy(buf, input, MAX_LINE - 1);
    buf[MAX_LINE - 1] = 0;

    char *eq = strchr(buf, '=');
    if (!eq) return -1;
    *eq = 0;
    char *lhs = buf, *rhs = eq + 1;
    strip(lhs); strip(rhs);

    p->op = '+';
    const char ops[] = "+-*/";
    for (const char *o = ops; *o; ++o)
        if (strchr(lhs, *o)) { p->op = *o; break; }

    p->lhs_n = 0;
    char *save = NULL;
    char *tok = strtok_r(lhs, "+-*/", &save);
    while (tok && p->lhs_n < MAX_WORDS - 1) {
        strip(tok);
        strncpy(p->lhs[p->lhs_n], tok, MAX_WORD_LEN - 1);
        p->lhs[p->lhs_n][MAX_WORD_LEN - 1] = 0;
        p->lhs_n++;
        tok = strtok_r(NULL, "+-*/", &save);
    }
    if (p->lhs_n < 2) return -1;

    strip(rhs);
    strncpy(p->rhs, rhs, MAX_WORD_LEN - 1);
    p->rhs[MAX_WORD_LEN - 1] = 0;
    return 0;
}

static int index_of(const LetterSet *ls, char c) {
    for (int i = 0; i < ls->n; ++i)
        if (ls->letters[i] == c) return i;
    return -1;
}

static void build_letters(const Puzzle *p, LetterSet *ls) {
    ls->n = 0;
    memset(ls->leading, 0, sizeof(ls->leading));

    for (int i = 0; i < p->lhs_n; ++i) {
        char c = p->lhs[i][0];
        int idx = index_of(ls, c);
        if (idx < 0) { ls->letters[ls->n++] = c; idx = ls->n - 1; }
        ls->leading[idx] = 1;
    }
    {
        char c = p->rhs[0];
        int idx = index_of(ls, c);
        if (idx < 0) { ls->letters[ls->n++] = c; idx = ls->n - 1; }
        ls->leading[idx] = 1;
    }
    for (int i = 0; i < p->lhs_n; ++i)
        for (const char *s = p->lhs[i]; *s; ++s)
            if (index_of(ls, *s) < 0) ls->letters[ls->n++] = *s;
    for (const char *s = p->rhs; *s; ++s)
        if (index_of(ls, *s) < 0) ls->letters[ls->n++] = *s;
}

static long long word_value(const char *word, const int *digit, const LetterSet *ls) {
    long long v = 0;
    for (const char *s = word; *s; ++s)
        v = v * 10 + digit[index_of(ls, *s)];
    return v;
}

static int evaluate(const Puzzle *p, const LetterSet *ls, const int *digit,
                    long long *lhs_out, long long *rhs_out) {
    long long vals[MAX_WORDS] = {0};
    for (int i = 0; i < p->lhs_n; ++i)
        vals[i] = word_value(p->lhs[i], digit, ls);

    long long left = 0;
    switch (p->op) {
        case '+':
            for (int i = 0; i < p->lhs_n; ++i) left += vals[i];
            break;
        case '-':
            left = vals[0];
            for (int i = 1; i < p->lhs_n; ++i) left -= vals[i];
            break;
        case '*':
            left = 1;
            for (int i = 0; i < p->lhs_n; ++i) left *= vals[i];
            break;
        case '/':
            left = vals[0];
            for (int i = 1; i < p->lhs_n; ++i) {
                if (vals[i] == 0 || left % vals[i] != 0) return 0;
                left /= vals[i];
            }
            break;
        default: return 0;
    }
    long long right = word_value(p->rhs, digit, ls);
    *lhs_out = left; *rhs_out = right;
    return left == right;
}

static void format_solution(const Puzzle *p, const LetterSet *ls,
                            const int *digit, char *out, size_t out_sz) {
    size_t pos = 0;
    for (int i = 0; i < p->lhs_n; ++i) {
        if (i) pos += snprintf(out + pos, out_sz - pos, " %c ", p->op);
        pos += snprintf(out + pos, out_sz - pos, "%lld",
                        word_value(p->lhs[i], digit, ls));
    }
    snprintf(out + pos, out_sz - pos, " = %lld", word_value(p->rhs, digit, ls));
}

/* ---------------- v2: сортировка букв ---------------- */

static void sort_letters_by_freq(const Puzzle *p, LetterSet *ls) {
    int freq[MAX_LETTERS] = {0};
    for (int i = 0; i < p->lhs_n; ++i)
        for (const char *s = p->lhs[i]; *s; ++s) freq[index_of(ls, *s)]++;
    for (const char *s = p->rhs; *s; ++s) freq[index_of(ls, *s)]++;

    for (int i = 1; i < ls->n; ++i) {
        char cl = ls->letters[i];
        int fl = ls->leading[i], fq = freq[i];
        int j = i - 1;
        while (j >= 0) {
            int better = (ls->leading[j] != fl)
                       ? ls->leading[j] < fl
                       : freq[j] < fq;
            if (!better) break;
            ls->letters[j+1] = ls->letters[j];
            ls->leading[j+1] = ls->leading[j];
            freq[j+1] = freq[j];
            j--;
        }
        ls->letters[j+1] = cl;
        ls->leading[j+1] = fl;
        freq[j+1] = fq;
    }
}

static int bt_v2(const Puzzle *p, const LetterSet *ls, int *digit, int *used,
                 int idx, char *out, size_t out_sz) {
    if (idx == ls->n) {
        long long l, r;
        if (evaluate(p, ls, digit, &l, &r)) {
            format_solution(p, ls, digit, out, out_sz);
            return 1;
        }
        return 0;
    }
    for (int d = 0; d <= 9; ++d) {
        if (used[d]) continue;
        if (d == 0 && ls->leading[idx]) continue;
        digit[idx] = d; used[d] = 1;
        if (bt_v2(p, ls, digit, used, idx + 1, out, out_sz)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_v2(const char *puzzle, char *out, size_t out_sz) {
    Puzzle p;
    if (parse_puzzle(puzzle, &p) != 0) return -1;
    LetterSet ls; build_letters(&p, &ls);
    sort_letters_by_freq(&p, &ls);
    int digit[MAX_LETTERS] = {0}, used[10] = {0};
    return bt_v2(&p, &ls, digit, used, 0, out, out_sz) == 1 ? 0 : -1;
}

/* ---------------- v3: коэффициенты + границы ---------------- */

static int64_t coeff_arr[MAX_LETTERS];
static int64_t suffix_min[MAX_LETTERS + 1];
static int64_t suffix_max[MAX_LETTERS + 1];

static int bt_v3_linear(const Puzzle *p, const LetterSet *ls, int *digit,
                        int *used, int idx, int64_t partial,
                        char *out, size_t out_sz) {
    if (idx == ls->n) {
        if (partial != 0) return 0;
        format_solution(p, ls, digit, out, out_sz);
        return 1;
    }
    int64_t cf = coeff_arr[idx];
    int dmin = ls->leading[idx] ? 1 : 0;
    for (int d = dmin; d <= 9; ++d) {
        if (used[d]) continue;
        int64_t new_partial = partial + cf * (int64_t)d;
        int64_t target = -new_partial;
        if (target < suffix_min[idx + 1] || target > suffix_max[idx + 1])
            continue;
        digit[idx] = d; used[d] = 1;
        if (bt_v3_linear(p, ls, digit, used, idx + 1, new_partial, out, out_sz))
            return 1;
        used[d] = 0;
    }
    return 0;
}

static void sort_letters_by_abs_coeff(LetterSet *ls, int64_t *coeff) {
    for (int i = 1; i < ls->n; ++i) {
        char c = ls->letters[i];
        int  l = ls->leading[i];
        int64_t k = coeff[i];
        int j = i - 1;
        while (j >= 0 && llabs((long long)coeff[j]) < llabs((long long)k)) {
            ls->letters[j+1] = ls->letters[j];
            ls->leading[j+1] = ls->leading[j];
            coeff[j+1] = coeff[j];
            j--;
        }
        ls->letters[j+1] = c;
        ls->leading[j+1] = l;
        coeff[j+1] = k;
    }
}

static int solve_v3(const char *puzzle, char *out, size_t out_sz) {
    Puzzle p;
    if (parse_puzzle(puzzle, &p) != 0) return -1;
    LetterSet ls; build_letters(&p, &ls);

    if (p.op == '*' || p.op == '/') {
        sort_letters_by_freq(&p, &ls);
        int digit[MAX_LETTERS] = {0}, used[10] = {0};
        return bt_v2(&p, &ls, digit, used, 0, out, out_sz) == 1 ? 0 : -1;
    }

    for (int i = 0; i < ls.n; ++i) coeff_arr[i] = 0;

    if (p.op == '+') {
        for (int i = 0; i < p.lhs_n; ++i) {
            int len = (int)strlen(p.lhs[i]);
            int64_t place = 1;
            for (int k = len - 1; k >= 0; --k) {
                coeff_arr[index_of(&ls, p.lhs[i][k])] += place;
                place *= 10;
            }
        }
    } else {
        int len = (int)strlen(p.lhs[0]);
        int64_t place = 1;
        for (int k = len - 1; k >= 0; --k) {
            coeff_arr[index_of(&ls, p.lhs[0][k])] += place;
            place *= 10;
        }
        for (int i = 1; i < p.lhs_n; ++i) {
            int l = (int)strlen(p.lhs[i]);
            int64_t pl = 1;
            for (int k = l - 1; k >= 0; --k) {
                coeff_arr[index_of(&ls, p.lhs[i][k])] -= pl;
                pl *= 10;
            }
        }
    }
    {
        int len = (int)strlen(p.rhs);
        int64_t place = 1;
        for (int k = len - 1; k >= 0; --k) {
            coeff_arr[index_of(&ls, p.rhs[k])] -= place;
            place *= 10;
        }
    }

    sort_letters_by_abs_coeff(&ls, coeff_arr);

    for (int i = ls.n - 1; i >= 0; --i) {
        int64_t cf = coeff_arr[i];
        int dmin = ls.leading[i] ? 1 : 0;
        int dmax = 9;
        if (cf >= 0) {
            suffix_min[i] = suffix_min[i+1] + cf * dmin;
            suffix_max[i] = suffix_max[i+1] + cf * dmax;
        } else {
            suffix_min[i] = suffix_min[i+1] + cf * dmax;
            suffix_max[i] = suffix_max[i+1] + cf * dmin;
        }
    }

    int digit[MAX_LETTERS] = {0}, used[10] = {0};
    return bt_v3_linear(&p, &ls, digit, used, 0, 0, out, out_sz) == 1 ? 0 : -1;
}

/* ---------------- main ---------------- */

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    const char *tests[] = {
        "SEND + MORE = MONEY",
        "BE + BE = MOO",
        "BIG + CAT = LION",
        "TWO + TWO = FOUR",
        "FORTY + TEN + TEN = SIXTY",
        "ELEVEN + NINE + FIVE + FIVE = THIRTY",
        "NINE - FOUR = FIVE",
        "AB * CD = EFGH",
        "AB / C / D = E",
        NULL
    };

    char out[256];
    double total = 0.0;

    printf("=== rebus_v3 (coeff + bounds) ===\n");
    for (int i = 0; tests[i]; ++i) {
        double t0 = now_ms();
        int rc = solve_v3(tests[i], out, sizeof(out));
        double dt = now_ms() - t0;
        total += dt;
        if (rc == 0)
            printf("%-42s -> %-38s %8.3f ms\n", tests[i], out, dt);
        else
            printf("%-42s -> %-38s %8.3f ms\n", tests[i], "(no solution)", dt);
    }
    printf("-------------------------------------------\n");
    printf("Total: %.3f ms\n", total);
    return 0;
}
