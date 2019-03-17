
recipe = {
    "create_hint" : {
        "params" : {
            "result_hint" : { "suppress_for_call" : True, },
            "implines" : { "suppress_for_call" : True, },
        },
        "return" : {
            "type" : "PyObject *",
            "retexpr" : "Py_RETURN_NONE",
            "convertor" : "Hexrays_Hooks::handle_create_hint_output",
            "convertor_pass_args" : True,
        }
    }
}
