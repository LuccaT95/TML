// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __INPUT_H__
#define __INPUT_H__

#include <vector>
#include <set>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <algorithm>

#include "defs.h"
#include "dict.h"
#include "memory_map.h"

/**
 * input class contains input data. input can be one of three types: STDIN,
 * FILE or STRING. STDIN works as a STRING which is read from the standard input
 * FILE uses system's memory mapping to access file data.
 * STDIN and STRING inputs from command line options and repl queries are
 * allocated and are freed when input is deconstructed.
 * STRING inputs loaded from archives are pointed to file's mmap.
 */
struct input {
	enum type { STDIN, FILE, STRING } type_; // input type
	bool newseq = false; // input is understood as a new prog sequence
	size_t offset = 0;   // offset of this input from the beginning
	size_t pos = 0;      // position of the currently parsed lexeme
	lexemes l = {};      // lexemes scanned from the input data
	bool error = false;  // parse error in the input's data
	/**
	 * STDIN input constructor
	 * @param ns - if true this input would be added as a new sequence ({})
	 */
	input(bool ns = false) : type_(STDIN), newseq(ns), beg_(0), data_(0),
		size_(load_stdin()) {
		//COUT << "created stdin input *: " << beg_ << std::endl;
	}
	/**
	 * STRING input constructor - without allocation (input from a mmap)
	 * @param s - pointer to a utf8 encoded input data
	 * @param sz - size of the input data
	 * @param ns - set ns true for making the input a new prog sequence
	 */
	input(void* s, size_t sz, bool ns = false) : type_(STRING), newseq(ns),
		beg_((ccs) s), data_(beg_), size_(sz), allocated_(false) {
		//COUT << "created pointer input: " << beg_ << std::endl;
	}
	/**
	 * STRING input constructor - with allocation
	 * @param s - pointer to a utf8 encoded input data
	 * @param ns - set ns true for making the input a new prog sequence
	 */
	input(ccs s, bool ns = false) : type_(STRING), newseq(ns),
		beg_(strdup(s)),data_(beg_),size_(strlen(beg_)),allocated_(true)
	{	//COUT << "created string input *: " << s << std::endl;
	}
	/**
	 * FILE input constructor
	 * @param f - file name
	 * @param ns - set ns true for making the input a new prog sequence
	 */
	input(std::string f, bool ns = false); // FILE
	/**
	 * destructor frees allocated data if any
	 */
	~input();
	/**
	 * lex scans a lexeme in a data pointer s and iterates it
	 * @param s - pointer to the input data
	 * @return scanned lexeme
	 */
	lexeme lex(pccs s);
	/**
	 * scans input's data for lexemes
	 * @return scanned lexemes
	 */
	lexemes& prog_lex();
	/**
	 * checks if lexeme is in this input and sets l's offset into lr if true
	 * @param beg - +offset to the resulting range
	 * @param l - lexeme
	 * @param lr - resulting range if lexeme found
	 * @return true if lexeme found
	 */
	bool lexeme_pos(const size_t& beg, const lexeme& l, lexeme_range& lr) {
		if ((l[0] >= beg_ && l[0] < beg_ + size_)
		 || (l[1] >= beg_ && l[1] < beg_ + size_))
			return	lr[0] = l[0] - beg_ + beg,
				lr[1] = l[1] - beg_ + beg, true;
		return false;
	}
	/**
	 * returns next input or 0 if this is the last one in the fwd list
	 * @return next input
	 */
	input* next() { return next_.get(); }
	/**
	 * sets next input when this on is the last one and another one is added
	 * @param next input
	 */
	void set_next(std::unique_ptr<input> ni) { next_ = std::move(ni); }
	/**
	 * @return pointer to the beginning of the data
	 **/
	ccs begin() { return beg_; }
	/**
	 * @return current pointer to the data
	 **/
	ccs data() { return data_; }
	/**
	 * @return size of the input data
	 **/
	size_t size() { return size_; }
	/**
	 * sets offset of this input
	 * @param o offset
	 **/
	void set_offset(size_t o) { offset = o; }
	int_t get_int_t(ccs from, ccs to);
	void count_pos(ccs o, long& l, long& ch);
	bool parse_error(ccs offset, const char* err) {
		return parse_error(offset, err, offset);
	}
	bool parse_error(ccs offset, const char* err, ccs close_to, ccs ctx = 0);
	bool parse_error(ccs offset, const char* err, lexeme close_to);
	bool type_error( const char* err, lexeme l);
	bool type_error(ccs offset, const char* err, ccs close_to);

	static std::string file_read(std::string fname);
	static std::string file_read_text(::FILE *f);
	static std::string file_read_text(std::string fname);
	static off_t fsize(const char *fname);
	static off_t fsize(ccs s, size_t len);
private:
	memory_map mm_;
	ccs beg_  = 0;
	ccs data_ = 0;
	size_t size_ = 0;
	bool allocated_ = false;
	int fd_ = -1;
	std::unique_ptr<input> next_ = 0;
	size_t load_stdin() {
		ostringstream_t ss; ss << CIN.rdbuf();
		beg_ = (ccs) strdup((ws2s(ss.str())).c_str()),
		data_ = beg_,
		allocated_ = true;
		return ss.str().size();
	}
};

class inputs {
	std::unique_ptr<input> first_ = 0;
	input *last_ = 0;
	size_t size_ = 0;
public:
	void add(std::unique_ptr<input> in) {
		//COUT << "inputs size: " << size_ << " adding input: " << in.get() << " last: " << last_<< std::endl;
		input *inp = in.get();
		if (last_) last_->set_next(std::move(in));
		else first_ = std::move(in);
		last_ = inp;
		size_++;
	}
	input* add_stdin() {
		std::unique_ptr<input> in = std::make_unique<input>();
		input* ret = in.get();
		add(std::move(in));
		return ret;
	}
	input* add_file(std::string filename) {
		//COUT << "adding file input: " << filename << std::endl;
		std::unique_ptr<input> in = std::make_unique<input>(filename);
		input* ret = in.get();
		add(std::move(in));
		//COUT << "inputs size: " << size() << " this: " << this <<std::endl;
		return ret;
	}
	input* add_string(const string_t& str) {
		//COUT << "adding string input: " << to_string(str) << std::endl;
		std::unique_ptr<input> in =std::make_unique<input>(str.c_str());
		input* ret = in.get();
		add(std::move(in));
		//COUT << "inputs size: " << size() << " this: " << this <<std::endl;
		return ret;
	}
	input* add_string(const std::string& str) {
		//COUT << "adding string input: " << str << std::endl;
		std::unique_ptr<input> in = std::make_unique<input>(
			to_string_t(str).c_str());
		input* ret = in.get();
		add(std::move(in));
		//COUT << "inputs size: " << size() << " this: " << this <<std::endl;
		return ret;
	}
	input* first() const { return first_.get(); }
	input* last() const { return last_; }
	size_t size() const { return size_; }
	//is l in inputs? then point *in to the input and set l's offset into lr
	bool lexeme_pos(size_t& beg, const lexeme& l, input** in,
		lexeme_range& lr)
	{
		input* it = first_.get();
		while (it) {
			beg += sizeof(size_t) + 1;
			if (it->lexeme_pos(beg, l, lr)) return *in = it, true;
			beg += it->size() + 1;
			it = it->next();
		}
		return *in = 0, false;
	}
};

struct elem {
	enum etype {
		NONE, SYM, NUM, CHR, VAR, OPENP, CLOSEP, ALT, STR,
		EQ, NEQ, LEQ, GT, LT, GEQ, BLTIN, NOT, AND, OR,
		FORALL, EXISTS, UNIQUE, IMPLIES, COIMPLIES, ARITH,
		OPENB, CLOSEB, OPENSB, CLOSESB, UTYPE, BLTINMOD
	} type = NONE;
	t_arith_op arith_op = NOP;
	int_t num = 0; // NUM's number or BLTIN's forget/renew bits
	// The string that represents variants of this element.
	lexeme e{ 0, 0 };
	char32_t ch = 0;
	elem() {};
	elem(int_t num) : type(NUM), num(num) {}
	elem(char32_t ch) : type(CHR), ch(ch) {}
	elem(etype type) : type(type) {
		switch(type) {
			case EQ: e = str2lexeme("="); break;
			case OPENP: e = str2lexeme("("); break;
			case CLOSEP: e = str2lexeme(")"); break;
			case ALT: e = str2lexeme("|"); break;
			case NEQ: e = str2lexeme("!="); break;
			case LEQ: e = str2lexeme("<="); break;
			case GT: e = str2lexeme(">"); break;
			case LT: e = str2lexeme("<"); break;
			case GEQ: e = str2lexeme(">="); break;
			case NOT: e = str2lexeme("~"); break;
			case AND: e = str2lexeme("&&"); break;
			case OR: e = str2lexeme("||"); break;
			case FORALL: e = str2lexeme("forall"); break;
			case EXISTS: e = str2lexeme("exists"); break;
			case UNIQUE: e = str2lexeme("unique"); break;
			case IMPLIES: e = str2lexeme("->"); break;
			case COIMPLIES: e = str2lexeme("<->"); break;
			case OPENB: e = str2lexeme("{"); break;
			case CLOSEB: e = str2lexeme("}"); break;
			case OPENSB: e = str2lexeme("["); break;
			case CLOSESB: e = str2lexeme("]"); break;
			default: assert(false); //should never reach here
		}
	}
	elem(bool b) {
		num = b;
		type = NUM;
	}
	elem(etype type, lexeme e) : type(type), e(e) {
		DBG(assert(type!=NUM&&type!=CHR&&(type!=SYM||(e[0]&&e[1])));)
	}
	elem(etype type, t_arith_op arith_op, lexeme e) : type(type),
			arith_op(arith_op), e(e) {
		DBG(assert(type!=NUM&&type!=CHR&&(type!=SYM||(e[0]&&e[1])));)
	}
	elem(t_arith_op arith_op) : type(ARITH), arith_op(arith_op) {
		switch(arith_op) {
			case MULT: e = str2lexeme("*"); break;
			case ADD:  e = str2lexeme("+"); break;
			default: assert(false); //should never reach here
		}
	}
	etype peek(input* in);
	bool is_paren() const { return type == OPENP || type == CLOSEP; }
	bool parse(input* in);
	bool operator<(const elem& t) const {
		if (type != t.type) return type < t.type;
		if (type == NUM) return num < t.num;
		if (type == CHR) return ch < t.ch;
		if (e[1]-e[0] != t.e[1]-t.e[0]) return e[1]-e[0]<t.e[1]-t.e[0];
		return strncmp(e[0], t.e[0], e[1]-e[0]) < 0;
	}
	bool operator==(const elem& t) const {
		if (type != t.type) return false;
		if (type == NUM) return num == t.num;
		if (type == CHR) return ch == t.ch;
		return e == t.e;
	}
	bool operator!=(const elem& t) const { return !(*this == t); }
	// Generate a fresh variable with respect to given dictionary.
	static elem fresh_var(dict_t &d) {
		return elem(elem::VAR, d.get_var_lexeme(d.get_new_var()));
	}
	// Generate a fresh symbol with respect to given dictionary.
	static elem fresh_sym(dict_t &d) {
		return elem(elem::SYM, d.get_sym_lexeme(d.get_new_sym()));
	}
	// Generate a fresh symbol with respect to given dictionary.
	static elem fresh_temp_sym(dict_t &d) {
		return elem(elem::SYM, d.get_temp_sym(d.get_fresh_temp_sym()));
	}
	std::string to_str() const { return to_string(to_str_t()); }
	string_t to_str_t() const {
		if (type == NUM) return to_string_t(num);
		if (type == CHR) return to_string_t(ch);
		return lexeme2str(e);
	}
};

//-----------------------------------------------------------------------------
// <-- This will be deprecated
// Type that uniquely identifies relations
typedef std::pair<lexeme, ints> signature;
bool operator<(const signature& m, const signature &n);
bool operator==(const signature& m, const signature &n);
template<> struct std::less<signature> {bool operator()(const signature&, const signature&)const;};
// -->

enum state_value { INIT, START, ADDS, DELS, RULE, COND, FP, CURR };
struct raw_term;
struct raw_rule;
struct raw_prog;
struct raw_form_tree;
typedef std::shared_ptr<raw_form_tree> sprawformtree;

//#ifdef BIT_UNIVERSE TODO: check whether this may affect early parser
struct context;
class environment;
typedef std::shared_ptr<struct context> spenvcontext;

struct primtype {
	elem el;
	int_t bsz = -1;
	enum _ptype {
		NOP, UINT, UCHAR, SYMB
	} ty = NOP;
	bool parse(input *in, const raw_prog& prog);
	bool operator==(const primtype& r) const {
		return ty == r.ty && bsz == r.bsz;
	};
	primtype(_ptype _ty = NOP) : ty(_ty) {}
	bool operator!=(const primtype& r) const {
		return !(*this == r);
	}
	std::string to_print() const{
		std::string s;
		switch(ty){
			case UINT: s.append("int"); break;
			case UCHAR: s.append("char"); break;
			case SYMB: s.append("sym"); break;
			default: return s.append("error_type");break;
		}
		if(bsz>0) {
			std::stringstream ss;
			ss<<bsz;
			//s.append(":");
			s.append(ss.str());
		}
		return s;
	}
	int_t get_maxbits() const {
		return 64;
	}
	size_t get_bitsz() const {
		switch(ty){
			case UINT: return bsz > 0 && bsz <= get_maxbits() ? bsz: 8;
			case UCHAR: return 8;
			case SYMB: return 8;
			default: return 0;
		}
	}
};

struct structype {
	elem structname;
	std::vector<struct typedecl> membdecl;
	bool parse(input *in, const raw_prog& prog);
	size_t get_bitsz(const std::vector<struct typestmt> & t) {
		DBG(bitsz > -1 ?  COUT<<"optimz" : COUT<<"";)
		return (bitsz < 0)? bitsz = calc_bitsz(t) :  bitsz;
	}
	size_t get_bitsz(class environment &e){
		DBG(bitsz > -1 ? COUT<<"optimz": COUT<<"";)
		return (bitsz < 0)? bitsz = calc_bitsz(e) : bitsz;
	}
	private:
	int_t bitsz = -1;
	size_t calc_bitsz(const std::vector<struct typestmt> &);
	size_t calc_bitsz(class environment &);
};

struct typedecl {
	primtype pty;
	elem structname; // struct type
	std::vector<elem> vars;
	bool is_primitive() const{
		DBG(assert(structname.e[0] == NULL || pty.ty == primtype::NOP));
		return pty.ty != primtype::NOP;
	}
	bool is_usertype() const{
		DBG(assert(structname.e[0] == NULL || pty.ty == primtype::NOP));
		return structname.e[0] != NULL;
	}
	size_t get_param_count() const {
		return vars.size();
	}
	std::string to_print() const {
		std::string ret;
		if(is_primitive())
			ret.append(pty.to_print());
		else ret.append(structname.to_str());
		for( const elem &e :  vars )
			ret.append(" "), ret.append(e.to_str());
		return ret;
	}

	bool parse(input *in , const raw_prog& prog, bool multivar = true);
};

struct typestmt {
	structype rty;
	elem reln;
	std::vector<typedecl> typeargs;
	bool is_predicate() const {
		DBG(assert( reln.e[0] != NULL || rty.structname.e[0] != NULL ));
		return reln.e[0] != NULL;
	}
	bool is_typedef() const {
		DBG(assert( reln.e[0] != NULL || rty.structname.e[0] != NULL ));
		return rty.structname.e[0] != NULL;
	}
	bool parse(input *in, const raw_prog& prog);

};
//#endif

/* A raw term is produced from the parsing stage. In TML source code, it
 * takes the following form: <rel>(<arg1> <arg2> ... <argN>). A raw term can
 * occur in both heads and bodies. For example, rel(a(b(c)d e)f) is a raw
 * term with the following elements: 'rel', '(', 'a', '(', 'b', '(', 'c', 'd',
 * 'e', ')', 'f', ')'. Interpreting terms in this way keeps the universe's
 * size finite which in turn guarantees that TML programs terminate. */

struct raw_term {

	bool neg = false;
	enum rtextype { REL, EQ, LEQ, BLTIN, ARITH, CONSTRAINT, VAR, FORM1, FORM2 } extype = raw_term::REL;

	//NOTE: we can add FORM1, FORM2 etc to rtextype
	// and replace t_arith_op by a form (once we do parse for compound arithmetic formulas)
	t_arith_op arith_op = NOP;
	// Elements of the raw term as described above.
	std::vector<elem> e;
	// A list formed from the raw-term's string by replacing opening parentheses
	// with -1s, closing parentheses with -2s, and contiguous sequences of elements
	// with their cardinality.
	ints arity;
	static bool require_fp_step;

	#ifdef TML_NATIVES
	sig s;
	#endif

	raw_term() {}
	raw_term(const elem &rel_name, const std::set<elem> &args) {
		e = { rel_name, elem(elem::OPENP) };
		for(const elem &a : args) {
			e.push_back(a);
		}
		e.push_back(elem(elem::CLOSEP));
		calc_arity(nullptr);
	}
	raw_term(const elem &rel_name, const std::vector<elem> &args) {
		e = { rel_name, elem(elem::OPENP) };
		for(const elem &a : args) {
			e.push_back(a);
		}
		e.push_back(elem(elem::CLOSEP));
		calc_arity(nullptr);
	}
	raw_term(const std::vector<elem> &f) : e(f) { calc_arity(nullptr); }
	raw_term(rtextype et, const std::vector<elem> &f) : extype(et), e(f) { calc_arity(nullptr); }
	raw_term(rtextype et, t_arith_op arith_op, const std::vector<elem> &f) : extype(et), arith_op(arith_op), e(f) { calc_arity(nullptr); }
	raw_term negate() const {
		raw_term nrt = *this;
		nrt.neg = !nrt.neg;
		return nrt;
	}
	bool parse(input* in, const raw_prog& prog, bool is_form = false,
		rtextype pref_type = raw_term::REL);
	bool calc_arity(input* in);
	int_t get_formal_arity() const;
	void add_parenthesis();
	void clear() { e.clear(), arity.clear(); }
	bool operator==(const raw_term& t) const {
		return neg == t.neg && e == t.e && arity == t.arity &&
			extype == t.extype;
	}
};

bool operator<(const raw_term& x, const raw_term& y);

struct macro {
	raw_term def;
	std::vector<raw_term> b;
	bool parse(input* in, const raw_prog& prog);
};

struct directive {
	elem rel;
	lexeme arg;
	raw_term t;
	int_t n;

	elem domain_sym; // Formal name of a relation containing a domain
	elem eval_sym; // Formal name of a relation containing an interpreter
	elem codec_sym; // Formal name of a relation containing a codec
	elem quote_sym; // Formal name of a relation containing a quotation
	elem limit_num; // The maximum of domain tuple elements
	elem arity_num; // The maximum length of domain tuples
	elem timeout_num; // The number of database steps to be simulated
	elem quote_str; // The literal string to be quoted.
	raw_term internal_term; // The term whose relation should be made internal

	enum etype { STR, FNAME, CMDLINE, STDIN, STDOUT, TREE, TRACE, BWD,
		EVAL, QUOTE, EDOMAIN, CODEC, INTERNAL, CMDLINEFILE } type;
	bool parse(input* in, const raw_prog& prog);
	bool operator==(const directive &b) const;
};

struct production {
	std::vector<elem> p;
	std::vector<raw_term> c{};   // constraints after production
	bool parse(input* in, const raw_prog& prog);
	bool operator<(const production& t) const { return p < t.p && c < t.c; }
	std::string to_str(size_t i=1 ){
		std::string ret;
		for( auto e = p.begin()+i; e != p.end(); e++)
			ret.append(e->to_str());
		return ret;
	}
};

bool operator==(const std::vector<raw_term>& x, const std::vector<raw_term>& y);

struct raw_prefix {
	elem qtype;
	elem ident;
	bool isfod = false;
	bool parse(input* in);
};

struct raw_form_tree {
	elem::etype type;
	std::optional<raw_term> rt; // elem::NONE is used to identify it
	std::optional<elem> el;

	sprawformtree l = nullptr;
	sprawformtree r = nullptr;
	bool neg = false;
	lexeme guard_lx = {0,0};

	// Make formula tree representing a single term. Canonize by always
	// extracting the negation from the term
	raw_form_tree (const raw_term &_rt) {
		if(_rt.neg) {
			type = elem::NOT;
			el = elem(elem::NOT);
			l = std::make_shared<raw_form_tree>(_rt.negate());
		} else {
			type = elem::NONE;
			rt = raw_term(_rt);
		}
	}
	// Make a formula tree with the given element and two children
	raw_form_tree (const elem &_el, sprawformtree _l = nullptr,
		sprawformtree _r = nullptr) :
		type(_el.type), el(_el),
		l(_l ? std::make_shared<raw_form_tree>(*_l) : nullptr),
		r(_r ? std::make_shared<raw_form_tree>(*_r) : nullptr) {}

	// Make a deep copy of the given formula tree
	raw_form_tree(const raw_form_tree &rft) : type(rft.type), rt(rft.rt),
		el(rft.el), l(rft.l ? std::make_shared<raw_form_tree>(*rft.l) : nullptr),
		r(rft.r ? std::make_shared<raw_form_tree>(*rft.r) : nullptr), neg(rft.neg),
		guard_lx(rft.guard_lx) {}

	// Move the given tree into this
	raw_form_tree(raw_form_tree &&rft) : type(rft.type), rt(rft.rt),
			el(rft.el), l(rft.l), r(rft.r), neg(rft.neg), guard_lx(rft.guard_lx) {
		rft.l = rft.r = nullptr;
	}

	// Make a deep copy of the given formula tree
	raw_form_tree &operator=(const raw_form_tree &rft) {
		type = rft.type;
		rt = rft.rt;
		el = rft.el;
		l = rft.l ? std::make_shared<raw_form_tree>(*rft.l) : nullptr;
		r = rft.r ? std::make_shared<raw_form_tree>(*rft.r) : nullptr;
		neg = rft.neg;
		guard_lx = rft.guard_lx;
		return *this;
	}

	// Move the given tree into this
	raw_form_tree &operator=(raw_form_tree &&rft) {
		type = rft.type;
		rt = rft.rt;
		el = rft.el;
		l = rft.l;
		r = rft.r;
		neg = rft.neg;
		guard_lx = rft.guard_lx;

		rft.l = rft.r = nullptr;
		return *this;
	}
	/* Puts the formulas parented by a tree of associative binary operators
	 * into a flat list. */
	void flatten_associative(const elem::etype &tp,
			std::vector<const raw_form_tree *> &tms) const {
		if(type == tp) {
			l->flatten_associative(tp, tms);
			r->flatten_associative(tp, tms);
		} else tms.push_back(this);
	}
	std::vector<const raw_form_tree *> flatten_associative(const elem::etype &tp) const {
		std::vector<const raw_form_tree *> tms;
		flatten_associative(tp, tms);
		return tms;
	}
	void printTree(int level =0 ) const;
	// Recursively check equality of formula trees
	bool operator==(const raw_form_tree &pft) const {
		// Types must be equal
		if(type != pft.type) return false;
		// Signs must be equal
		else if(neg != pft.neg) return false;
		// If formulas are terms, then they must be equal
		else if(type == elem::NONE) return *rt == *pft.rt;
		// Either both elements are defined or both are not
		else if(bool(el) != bool(pft.el)) return false;
		// Either both elements are undefined or both are equal
		else if(el && *el != *pft.el) return false;
		// Either both left trees are defined or both are not
		else if(bool(l) != bool(pft.l)) return false;
		// Either both left trees are undefined or both are equal
		else if(l != nullptr && *l != *pft.l) return false;
		// Either both right trees are defined or both are not
		else if(bool(r) != bool(pft.r)) return false;
		// Either both right trees are undefined or both are equal
		else if(r != nullptr && *r != *pft.r) return false;
		// So both operands are equal
		else return true;
	}
	// Recursively check order of formula trees
	bool operator<(const raw_form_tree &pft) const {
		// Types most significant lexicographically
		if(type != pft.type) return type < pft.type;
		// Signs next significant
		else if(neg != pft.neg) return neg < pft.neg;
		// If formulas are terms, then terms next significant
		else if(type == elem::NONE) return *rt < *pft.rt;
		// Then element definedness next significant
		else if(bool(el) != bool(pft.el)) return bool(el) < bool(pft.el);
		// If both defined, then element contents next significant
		else if(el && *el != *pft.el) return *el < *pft.el;
		// Then left tree definedness next significant
		else if(bool(l) != bool(pft.l)) return bool(l) < bool(pft.l);
		// Then left tree contents next significant
		else if(l != nullptr && *l != *pft.l) return *l < *pft.l;
		// Then right tree definedness next significant
		else if(bool(r) != bool(pft.r)) return bool(r) < bool(pft.r);
		// Then right tree contents next significant
		else if(r != nullptr && *r != *pft.r) return *r < *pft.r;
		// Otherwise first operand cannot preceed second
		else return false;
	}
	// Check formula tree inequality by checking equality
	bool operator!=(const raw_form_tree &pft) {
		return !(*this == pft);
	}
};

struct raw_rule {
	std::vector<raw_term> h;
	// An empty b signifies the presence of a logical formula in prft if
	// prft != nullopt, otherwise it signifies that this rule is a fact.
	std::vector<std::vector<raw_term>> b;
	// Contains a tree representing the logical formula.
	std::optional<raw_form_tree> prft;

	// contains the context types of vars used in rule from type inference
	mutable spenvcontext varctx = nullptr;

	enum etype { NONE, GOAL, TREE };
	etype type = NONE;
	bool guarding = false;
	bool parse(input* in, const raw_prog& prog);
	void clear() { h.clear(), b.clear(), type = NONE; }
	raw_rule(){}
	raw_rule(etype type, const raw_term& t) : h({t}), type(type) {}
	raw_rule(const raw_term& t) : raw_rule(NONE, t) {}
	raw_rule(const raw_term& h, const raw_term& b) : h({h}), b({{b}}) {}
	raw_rule(const raw_term& h, const std::vector<raw_term>& _b) : h({h}) {
		if (!_b.empty()) b = {_b};
	}
	raw_rule(const raw_term& h, const std::vector<std::vector<raw_term>>& _b) : h({h}), b(_b) {}
	raw_rule(const std::vector<raw_term> &h,
			const std::vector<raw_term>& _b) : h(h) {
		if (!_b.empty()) b = {_b};
	}
	raw_rule(const raw_term& h, const raw_form_tree &prft) : h({h}), prft(prft) {}
	raw_rule(const std::vector<raw_term> &h, const raw_form_tree &prft) : h(h), prft(prft) {}
	raw_rule(const std::vector<raw_term> &h, const sprawformtree &prft) : h(h), prft(*prft) {}

	void update_context(const spenvcontext &_c) const {
		varctx = _c;
	}
	spenvcontext get_context() const {
		return varctx;
	}
	void update_states(std::array<bool, 8>& has) {
		if (is_form() || is_dnf()) has[RULE] = true;
		else for (auto hi : h) has[hi.neg ? DELS : ADDS] = true;
	}
	inline bool is_dnf() const
		{ return type == NONE && b.size() > 0; }
	inline bool is_form() const
		{ return type == NONE && prft && b.size() == 0; }
	inline bool is_fact() const
		{ return type == NONE && b.size() == 0 && !prft; }
	inline bool is_goal() const
		{ return type == GOAL && b.size() == 0 && !prft; }
	// If prft not set, convert b to prft, then return prft
	std::optional<raw_form_tree> get_prft() const;
	raw_rule try_as_prft() const;
	// Clear b and set prft
	raw_form_tree &set_prft(const raw_form_tree &_prft) {
		b.clear();
		return *(prft = _prft);
	}
	// Clear prft and set b
	std::vector<std::vector<raw_term>>
			&set_b(const std::vector<std::vector<raw_term>> &_b) {
		prft.reset();
		return b = _b;
	}
	std::optional<std::vector<std::vector<raw_term>>> get_b() const;
	static raw_rule getdel(const raw_term& t) {
		raw_rule r(t, t);
		return r.h[0].neg = true, r;
	}
	raw_rule try_as_b() const;
	bool operator==(const raw_rule& r) const {
		if(h != r.h) return false;
		else if(is_form() != r.is_form()) return false;
		else if(is_form()) return *prft == *r.prft;
		else return b == r.b;
	}
	bool operator!=(const raw_rule& r) const { return !(*this == r); }
};

struct raw_sof {
	const raw_prog& prog;
	raw_sof(const raw_prog& prog) :prog(prog) {}
private:
	bool parseform(input* in, sprawformtree &root, int precd= 0);
	bool parsematrix(input* in, sprawformtree &root);
public:
	bool parse(input* in, sprawformtree &root);
};

struct guard_statement {
	enum gtype { IF, WHILE } type;
	bool parse_condition(input* in, raw_prog& rp);
	bool parse_if(input* in, dict_t &dict, raw_prog& rp);
	bool parse_while(input* in, dict_t &dict, raw_prog& rp);
	bool parse(input* in, dict_t &dict, raw_prog& rp);
	std::optional<raw_form_tree> prft;       // condition
	int_t rp_id = 0;          // prog id in which to run the condition
	int_t true_rp_id  = 0;    // id of a true  prog
	int_t false_rp_id = 0;    // id of a false prog
	raw_prog* p_break_rp = 0; // ptr to a prog to break if while cond. true
};

struct state_block;

struct raw_prog {
	enum ptype {
		PFP, LFP, GFP
	} type = PFP;
	std::reference_wrapper<dict_t> dict;
	std::vector<macro> macros;
	std::vector<directive> d;
	std::vector<production> g;
	std::vector<raw_rule> r;

	std::vector<guard_statement> gs;
	std::vector<typestmt> vts;
	std::vector<raw_prog> nps;
	std::vector<state_block> sbs;

	// The relations that should be hidden from the user by default
	std::set<signature> hidden_rels;
	int_t id = 0;
	int_t guarded_by = -1;
	int_t true_rp_id = -1;
	std::array<bool, 8> has = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static int_t last_id;
	static bool require_guards;
	static bool require_state_blocks;

	bool parse(input* in);
	bool parse_statement(input* in);
	bool parse_nested(input* in);
	bool parse_xfp(input* in);
	bool expand_macros(input* in);
	bool macro_expand(input *in, macro mm, const size_t i, const size_t j, 
				std::vector<raw_term> &vrt);
	raw_prog(dict_t &dict_) : dict(dict_) {};
};

struct raw_progs {
	std::reference_wrapper<dict_t> dict;
	raw_prog p;
	bool parse(input* in);
	raw_progs(dict_t &dict_) : dict(dict_), p(dict_) { };
};

struct state_block {
	bool flip = false;
	lexeme label;
	raw_prog p;
	state_block(dict_t &dict_) : p(dict_) {};
	bool parse(input* in);
};

bool throw_runtime_error(std::string err, std::string details = "");

bool parse_error(const char* o, const char* e, ccs s);
bool parse_error(const char* o, const char* e, lexeme l);
bool parse_error(ccs o, const char* e, std::string s);
bool parse_error(ccs o, const char* e);
bool parse_error(const char* e, lexeme l);
bool parse_error(const char* e, lexeme l, std::string s);
bool parse_error(const char* e);
bool type_error(const char* e, lexeme l);

std::string to_string(const raw_term& rt, std::string delim="", int_t skip=0);

template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const directive& d);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const elem& e);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const raw_form_tree &t);
#ifdef DEBUG
template <typename T>
std::basic_ostream<T>& print_raw_form_tree(std::basic_ostream<T>& os, const raw_form_tree &t, bool root = true);
#endif // DEBUG
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const raw_term& t);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const sprawformtree prts);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const std::pair<elem, bool>& p);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const std::tuple<raw_term, std::string, int_t>& p);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const raw_rule& r);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const raw_prog& p);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const raw_progs& p);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const production& p);
std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const lexeme& l);
std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, const lexeme& l);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const std::set<raw_term>& rts);
template <typename T>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, const std::vector<raw_term>& rts);

template <typename T, typename VT>
std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os,
	const std::vector<VT>& v);

// print elements to an output (and skip args if to or delimited)
// if to then output name is the first argument (default: @output)
// if delimited then delimiter string is the next argument (default: "")
ostream_t& print_to_delimited(const raw_term& rt, bool& error, bool to = false,
	bool delimited = false);

template <typename T>
std::basic_ostream<T>& print_raw_prog_tree(std::basic_ostream<T>& os,
	const raw_prog& p, size_t level);
template <typename T>
std::basic_ostream<T>& print_raw_rule(std::basic_ostream<T>& os,
	const raw_rule& r, size_t level);

bool operator<(const raw_rule& x, const raw_rule& y);

#endif
