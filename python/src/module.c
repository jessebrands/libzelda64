/*
 * module.c: Python module entrypoint
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libzelda64.
 *
 * libzelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libzelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libzelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <zelda64/zelda64.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

PyDoc_STRVAR(module_doc,
             "Low-level bindings to libzelda64.\n"
             "\n"
             "This module wraps the C API an its interface is not stable.\n"
             "Import zelda64 instead.");

PyMODINIT_FUNC PyInit__zelda64(void);

static PyMethodDef module_methods[] = {
    {NULL, NULL, 0, NULL},
};

static int
module_exec(PyObject* module) {
    if (PyModule_AddStringConstant(module, "__version__", ZELDA64_VERSION_STRING) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, (void*) module_exec},
    {0, NULL},
};

static struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "zelda64._zelda64",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = module_methods,
    .m_slots = module_slots,
};

PyMODINIT_FUNC PyInit__zelda64(void) {
    return PyModuleDef_Init(&module_def);
}
