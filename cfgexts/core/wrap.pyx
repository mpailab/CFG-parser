# cython: c_string_type=unicode, c_string_encoding=utf8

from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map as cpp_map
from libc.stdlib cimport malloc, free
from cpython.long cimport PyLong_FromVoidPtr as from_cptr
from cpython.long cimport PyLong_AsVoidPtr as to_cptr

from wrap cimport *
from wrap cimport quasiquote as c_quasiquote
from wrap cimport ParseNode as CParseNode
from wrap cimport ParseContext as CParseContext
from wrap cimport ast_to_text as c_ast_to_text
from cython.operator cimport dereference as deref

from core.python_grammar import python_grammar_str
cdef string python_grammar = python_grammar_str

cdef inline to_bytes (text):
    return text.encode('utf-8')

cdef inline to_str (text):
    return text.decode('utf-8')

def apply (text):
    return to_str(c_apply(to_bytes(text)))

def load_file(filename):
    return c_loadFile(filename)

def pn_equal (pn1, pn2):
    return equal_subtrees(<CParseNode*> to_cptr(pn1), <ParseNode*> to_cptr(pn2))

def add_rule (px, lhs, rhs):
    cdef ParseContext* c_px = <ParseContext*> to_cptr(px)
    return addRule(c_px.grammar(), to_bytes(lhs + " -> " + rhs))



# def ast_to_text (px, pn):
#     return to_str(c_ast_to_text(<ParseContext*> to_cptr(px), <ParseNode*> to_cptr(pn)))

__parse_context__ = None
def parse_context():
    global __parse_context__
    return __parse_context__

cdef class ParseNode:
    cdef CParseNode* p

    def __init__(self, cnode):
        self.p = cnode
        if self.p:
            self.p.refs += 1

    def __del__(self):
        if self.p:
            self.p.refs -= 1

    cpdef num_children(self) -> int:
        return self.p.ch.size()

    cpdef __getitem__(self, int i):
        if i < 0 or i >= self.p.ch.size():
            raise ValueError(f"Parse node child index {i} out of range ({self.p.ch.size()})")
        return ParseNode(self.p.ch[i])

    cpdef void __setitem__(self, int i, ParseNode value):
        if i < 0 or i >= self.p.ch.size():
            raise ValueError(f"Parse node child index {i} out of range ({self.p.ch.size()})")
        self.p.ch[i] = value.p

    @property
    cpdef vector[CParseNode*] children(self):
        return self.p.ch

    @property
    cpdef int rule(self):
        return self.p.rule

    @property
    cpdef const string& str(self):
        assert self.p.isTerminal()
        return self.p.term

    @property
    cpdef bool is_terminal(self):
        return self.p.isTerminal()

    @property
    cpdef int ntnum(self):
        return self.p.nt

    def __eq__(self, other):
        assert type(other) is ParseNode
        return pn_equal(self.p, other.p) != 0

    def __repr__(self):
        return 'ParseNode:\n'+parse_context().ast_to_text(self)


cdef class ParseContext:
    cdef CParseContext* px
    cdef cpp_map[int, object] syntax_rules
    cdef cpp_map[int, object] macro_rules
    cdef cpp_map[int, object] token_rules
    # cdef dict lexer_rules
    cdef list exported_syntax
    cdef list exported_macro
    cdef list exported_tokens
    cdef list exported_lexer_rules

    def __init__(self):
        self.px = create_python_context(True, python_grammar)
        # self.syntax_rules = {}
        # self.macro_rules = {}
        # self.token_rules = {}
        # self.lexer_rules = {}
        self.exported_syntax = []
        self.exported_macro = []
        self.exported_tokens = []
        self.exported_lexer_rules = []

    # def eval(self, expr):
    #     if type(expr) is ParseNode:
    #         expr = ast_to_text(self, expr)
    #     return eval(expr, self.globals)

    cpdef syntax_function(self, int rule):
        it = self.syntax_rules.find(rule)
        if it == self.syntax_rules.end():
            return None
        return deref(it).second
        #return self.syntax_rules.get(rule, None)

    cpdef macro_function(self, int rule):
        it = self.macro_rules.find(rule)
        if it == self.macro_rules.end():
            return None
        return deref(it).second

    cpdef token_function(self, int rule):
        it = self.token_rules.find(rule)
        if it == self.token_rules.end():
            return None
        return deref(it).second

    def __enter__(self):
        global __parse_context__
        if __parse_context__ is not None:
            raise Exception('Enter parse context when previous context not deleted')
        __parse_context__ = self
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        global __parse_context__
        __parse_context__ = None

    def __del__(self):
        del_python_context(self.px)

    cpdef add_token(self, const string& name, const string& rhs, object apply=None, bool export=False):
        cdef int rule_id = add_token(self.px, name, rhs)
        self.token_rules[rule_id] = apply
        if export:
            self.exported_tokens.append((name, rhs, apply))

    cpdef add_lexer_rule(self, const string& lhs, const string& rhs, export=False):
        cdef int rule_id = add_lexer_rule(self.px, lhs, rhs)
        if export:
            self.exported_lexer_rules.append((lhs, rhs))

    def add_macro_rule(self, lhs: str, rhs: list, apply, export=False,
                         int lpriority=-1, int rpriority=-1):
        rhs = ' '.join(str(x) for x in rhs)
        # print(f'add rule: {lhs} -> {rhs}:',end='')
        cdef int rule_id = self._add_rule(lhs, rhs, lpriority, rpriority)
        # print(f' id = {rule_id}, f = {apply}')
        # print(f'add macro rule {rule_id}')
        self.macro_rules[rule_id] = apply
        if export:
            self.exported_macro.append((lhs, rhs, apply, lpriority, rpriority))

    def add_syntax_rule(self, lhs: str, rhs, apply, export=False, lpriority=-1, rpriority=-1):
        rhs = ' '.join(str(x) for x in rhs)
        cdef int rule_id = add_rule(self.px, lhs, rhs, lpriority, rpriority)
        self.syntax_rules[rule_id] = apply
        if export:
            self.exported_syntax.append((lhs, rhs, apply, lpriority, rpriority))

    def gen_syntax_import(self):
        res = "def _import_grammar(px: ParseContext):\n" \
              "  # во-первых, импортируем грамматику из всех подмодулей, если такие были\n" \
              "  if '_imported_syntax_modules' in globals():\n" \
              "    for sm in _imported_syntax_modules:\n" \
              "      if hasattr(sm, '_import_grammar'):\n" \
              "        sm._import_grammar(px)\n\n"
        for lhs, rhs, apply, lpr, rpr in self.exported_syntax:
            res += f'''  px.add_syntax_rule({lhs}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})\n'''
        for lhs, rhs, apply, lpr, rpr in self.exported_macro:
            res += f'''  px.add_macro_rule({lhs}, {repr(rhs)}, apply={apply.__name__}, lpriority={lpr}, rpriority={rpr})\n'''
        for lhs, rhs, apply in self.exported_tokens:
            res += f'''  px.add_token({lhs}, {repr(rhs)}, apply={apply.__name__ if apply else 'None'})\n'''
        for lhs, rhs in self.exported_lexer_rules:
            res += f'''  px.add_lexer_rule({lhs}, {repr(rhs)})\n'''
        return res

    cpdef ast_to_text(self, ParseNode pn):
        return c_ast_to_text(self.px, pn.p)

    cdef int _add_rule(self, lhs, rhs, lpr, rpr):
        return addRule(self.px.grammar(), lhs + " -> " + rhs, -1, lpr, rpr)


cdef class Parser:
    cdef PythonParseContext* px
    cdef ParserState* state
    def __init__(self, px: ParseContext, text: str):
        self.px = px.px
        self.state = new ParserState(px, text, "")

    def __del__(self):
        if self.state:
            del self.state

    def __iter__(self):
        return self

    def __next__(self):
        cdef CParseNode* root = self.state.parse_next().root.get()
        if not root:
            raise StopIteration
        return ParseNode(root)


def parse_gen(px, text):
    for node in Parser(px, text):
        yield node

def syn_expand(node: ParseNode):
    px = parse_context()
    if node.is_terminal:
        apply = px.token_function(node.ntnum)
        if apply is not None:
            return apply(node.str)
        return node.str

    # print(f'in syn_expand, rule = {node.rule}')
    f = px.syntax_function(node.rule)
    if not f:
        raise Exception(f'syn_expand: cannot find syntax expand function for rule {node.rule}')
    return f(*node.children)

cpdef macro_expand(ParseContext px, ParseNode node):
    """ Раскрывает макросы в синтаксическом дереве """
    while True:
        f = px.macro_function(node.rule)
        if f is None:
            break
        node = f(*node.children)

    cdef int i
    for i in range(node.num_children()):
        node[i] = macro_expand(px, node[i])
    return node

#ParseNode* quasiquote(ParseContext* px, const string& nt, const vector<string>& parts, const vector<ParseNode*>& subtrees);
def quasiquote(ntname, str_list, tree_list):
    assert len(str_list) == len(tree_list)+1
    cdef CParseContext* px = parse_context().px
    cdef vector[CParseNode*] subtrees
    cdef int i, n = len(tree_list)
    subtrees.resize(n)
    for i in range(n):
        subtrees[i] = tree_list[i].p
    cdef CParseNode* nn = c_quasiquote(px, ntname, str_list, subtrees)
    return ParseNode(nn)

def ast_to_text(px, pn):
    px.ast_to_text(pn)


class CppDbgFlags:
    SHIFT = 0x1
    REDUCE = 0x2
    STATE = 0x4
    LOOKAHEAD = 0x8
    TOKEN = 0x10
    RULES = 0x20
    QQ = 0x40
    ALL = 0xFFFFFFF


def set_debug(flags):
    set_cpp_debug(flags)
