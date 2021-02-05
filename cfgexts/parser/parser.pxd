
from libcpp.string cimport string
from cpython.ref cimport PyObject

cdef extern from "apply.h":
    string apply (string text)
    void loadFile(const string &filename) except +
    PyObject* c_quasiquote(PyObject* px, char* nt, int n, PyObject* data, PyObject* pn)
    PyObject* new_python_context(int by_stmt, string syntax_file)
    void del_python_context(PyObject*)
    void inc_pn_num_refs(void *pn)
    void dec_pn_num_refs(void *pn)
    int get_pn_num_children(PyObject* pn)
    PyObject* get_pn_child(PyObject* pn, int i)
    void set_pn_child(PyObject* pn, int i, PyObject* ch)
    int get_pn_rule(PyObject* pn)
    int pn_equal(PyObject* pn1, PyObject* pn2)
    int add_rule(PyObject* px, char* lhs, char *rhs)
    PyObject* new_parser_state(PyObject* px, const char* text, const char *start)
    PyObject* continue_parse(PyObject* state)
    void del_parser_state(PyObject* state)